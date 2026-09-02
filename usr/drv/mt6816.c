// ============================================================
// mt6816.c — MT6816 编码器驱动（usr/drv，同步读角）
//
// 芯片协议：SPI Mode3(CPOL=1,CPHA=1) / 16bit / 14bit，分辨率 16384
// 读角序列（一次 CS 周期两段，响应滞后一帧）：
//   段1 发送 0x83FF（读命令高字节段），响应为上一命令残留 → 弃用
//   段2 发送 0x84FF（读命令低字节段），响应为本次有效数据帧
// 解析：见 mt6816 协议（奇偶校验 + 磁场告警位）
//
// 仅依赖 usr/if 接口表与 enc_spi_engine，无任何板级/厂商库依赖。
// ============================================================

#include <stdlib.h>

#include "usr/abs/device.h"
#include "usr/abs/encoder.h"

#include "enc_spi_engine.h"
#include "encoder_drivers.h"

#define MT6816_RESOLUTION 16384U
#define MT6816_CMD_HIGH 0x83FFU
#define MT6816_CMD_LOW 0x84FFU
#define MT6816_MAG_WARN (1U << 1) // 磁场告警位（帧2 bit1）
#define MT6816_PARITY_BIT (1U << 0)

typedef struct
{
    eDeviceStatus dstate;
    tEncSpiEngine eng;

    uint16_t cmd_high; // 段1 tx
    uint16_t cmd_low;  // 段2 tx
    uint8_t rx1[2];    // 段1 rx（弃用）
    uint8_t rx2[2];    // 段2 rx（有效数据帧）

    uint16_t raw; // 最近一次有效角度
    uint32_t ts;  // 最近一次有效时间戳(ms)
} tMT6816_ctx;

// 8bit 奇偶（偶数个 1 → false；与协议中 popcount%2 语义一致）
static bool parity_odd(uint16_t v)
{
    v ^= (uint16_t)(v >> 8);
    v ^= (uint16_t)(v >> 4);
    v ^= (uint16_t)(v >> 2);
    v ^= (uint16_t)(v >> 1);
    return (v & 1U) != 0U;
}

// ---- ops 实现 ----

static bool MT6816_init(EncoderChipHandle h)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)h;
    if (!ctx)
        return false;

    // SPI Mode3(CPOL=1,CPHA=1)/16bit —— 每次 init 显式声明：
    // 同一总线上可能换芯片（如刚从 AS5047 切过来），必须把模式切回 Mode3
    if (!ctx->eng.bus->set_mode(ctx->eng.bus->ctx, 1U, 1U, 16U))
        return false;

    ctx->cmd_high = MT6816_CMD_HIGH;
    ctx->cmd_low = MT6816_CMD_LOW;
    ctx->raw = 0U;
    ctx->ts = 0U;
    ctx->dstate = DEV_ONLINE;
    return true;
}

static bool MT6816_read_angle(EncoderChipHandle h, uint16_t *raw, uint32_t *ts_ms)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)h;
    if (!ctx || !raw || !ts_ms)
        return false;

    tEncXferSeg segs[2];
    segs[0].tx = (const uint8_t *)&ctx->cmd_high;
    segs[0].rx = ctx->rx1;
    segs[0].len = 2U;
    segs[1].tx = (const uint8_t *)&ctx->cmd_low;
    segs[1].rx = ctx->rx2;
    segs[1].len = 2U;

    if (!enc_engine_run(&ctx->eng, segs, 2U))
    {
        ctx->dstate = DEV_RUN_ERROR;
        return false;
    }

    uint16_t high = (uint16_t)(ctx->rx1[0] | ((uint16_t)ctx->rx1[1] << 8));
    uint16_t low = (uint16_t)(ctx->rx2[0] | ((uint16_t)ctx->rx2[1] << 8));

    if (low & MT6816_MAG_WARN) // 磁场告警 → 数据不可信
        return false;

    // 奇偶校验：bits 内 1 的个数奇偶性须与帧 bit0 一致
    uint16_t bits = (uint16_t)(((high & 0x00FFU) << 7) | ((low & 0x00FEU) >> 1));
    bool parity = (low & MT6816_PARITY_BIT) != 0U;
    if (parity_odd(bits) != parity)
        return false;

    ctx->raw = bits >> 1; // 14bit
    ctx->ts = ctx->eng.time->get_ms(ctx->eng.time->ctx);
    ctx->dstate = DEV_RUNNING;
    *raw = ctx->raw;
    *ts_ms = ctx->ts;
    return true;
}

static bool MT6816_get_resolution(EncoderChipHandle h, uint16_t *res)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)h;
    if (!ctx || !res)
        return false;
    *res = MT6816_RESOLUTION;
    return true;
}

static void MT6816_reset(EncoderChipHandle h)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)h;
    if (!ctx)
        return;
    enc_engine_abort(&ctx->eng);
    ctx->raw = 0U;
    ctx->ts = 0U;
    ctx->dstate = DEV_ONLINE;
}

static uint8_t MT6816_get_state(EncoderChipHandle h)
{
    tMT6816_ctx *ctx = (tMT6816_ctx *)h;
    return (uint8_t)(ctx ? ctx->dstate : DEV_OFFLINE);
}

const tEncoderDriverOps MT6816_driver_ops = {
    .init = MT6816_init,
    .read_angle = MT6816_read_angle,
    .get_resolution = MT6816_get_resolution,
    .reset = MT6816_reset,
    .get_state = MT6816_get_state,
};

// ---- 句柄创建/销毁（资源注入点） ----

EncoderChipHandle MT6816_create(const tSpiBusIf *bus, const tTimeIf *time)
{
    if (!bus || !time)
        return NULL;

    tMT6816_ctx *ctx = (tMT6816_ctx *)calloc(1U, sizeof(tMT6816_ctx));
    if (!ctx)
        return NULL;

    ctx->dstate = DEV_OFFLINE;
    ctx->eng.bus = bus;
    ctx->eng.time = time;
    return (EncoderChipHandle)ctx;
}

void MT6816_destroy(EncoderChipHandle h)
{
    free(h);
}
