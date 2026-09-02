// ============================================================
// mt6835.c — MT6835 编码器驱动（usr/drv，同步读角）
//
// 芯片协议：SPI Mode3(CPOL=1,CPHA=1) / 8bit，分辨率 16384
// 读角序列（一次 CS 周期单段 5 字节，边发边收）：
//   发送 {0xA0, 0x03, 0x00, 0x00, 0x00}，同时接收 5 字节
//   角度 21bit：rx[2]<<13 | rx[3]<<5 | rx[4]>>3，输出取高 14 位
//   status 位（rx[4] bit0..2）：bit1 = 磁场太弱 → 数据不可信
//
// 仅依赖 usr/if 接口表与 enc_spi_engine，无任何板级/厂商库依赖。
// ============================================================

#include <stdlib.h>

#include "usr/abs/device.h"
#include "usr/abs/encoder.h"

#include "enc_spi_engine.h"
#include "encoder_drivers.h"

#define MT6835_RESOLUTION 16384U
#define MT6835_FRAME_LEN 5U
#define MT6835_MAG_WEAK_BIT 0x02U // status bit1：磁场太弱

// 连续读角度命令（8bit）
static const uint8_t MT6835_CMD[MT6835_FRAME_LEN] = {0xA0U, 0x03U, 0x00U, 0x00U, 0x00U};

typedef struct
{
    eDeviceStatus dstate;
    tEncSpiEngine eng;

    uint8_t rx[MT6835_FRAME_LEN]; // 接收缓冲（8bit 字节流）

    uint16_t raw; // 最近一次有效角度
    uint32_t ts;  // 最近一次有效时间戳(ms)
} tMT6835_ctx;

// ---- ops 实现 ----

static bool MT6835_init(EncoderChipHandle h)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)h;
    if (!ctx)
        return false;

    // SPI Mode3(CPOL=1,CPHA=1)/8bit —— 芯片协议事实
    if (!ctx->eng.bus->set_mode(ctx->eng.bus->ctx, 1U, 1U, 8U))
        return false;

    ctx->raw = 0U;
    ctx->ts = 0U;
    ctx->dstate = DEV_ONLINE;
    return true;
}

static bool MT6835_read_angle(EncoderChipHandle h, uint16_t *raw, uint32_t *ts_ms)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)h;
    if (!ctx || !raw || !ts_ms)
        return false;

    tEncXferSeg seg;
    seg.tx = MT6835_CMD;
    seg.rx = ctx->rx;
    seg.len = MT6835_FRAME_LEN;

    if (!enc_engine_run(&ctx->eng, &seg, 1U))
    {
        ctx->dstate = DEV_RUN_ERROR;
        return false;
    }

    // status 位：bit1 = 磁场太弱
    if (ctx->rx[4] & MT6835_MAG_WEAK_BIT)
        return false;

    // 21bit 角度 → 高 14 位输出
    uint32_t angle_21 = ((uint32_t)ctx->rx[2] << 13) |
                        ((uint32_t)ctx->rx[3] << 5) |
                        ((uint32_t)ctx->rx[4] >> 3);

    ctx->raw = (uint16_t)(angle_21 >> 7);
    ctx->ts = ctx->eng.time->get_ms(ctx->eng.time->ctx);
    ctx->dstate = DEV_RUNNING;
    *raw = ctx->raw;
    *ts_ms = ctx->ts;
    return true;
}

static bool MT6835_get_resolution(EncoderChipHandle h, uint16_t *res)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)h;
    if (!ctx || !res)
        return false;
    *res = MT6835_RESOLUTION;
    return true;
}

static void MT6835_reset(EncoderChipHandle h)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)h;
    if (!ctx)
        return;
    enc_engine_abort(&ctx->eng);
    ctx->raw = 0U;
    ctx->ts = 0U;
    ctx->dstate = DEV_ONLINE;
}

static uint8_t MT6835_get_state(EncoderChipHandle h)
{
    tMT6835_ctx *ctx = (tMT6835_ctx *)h;
    return (uint8_t)(ctx ? ctx->dstate : DEV_OFFLINE);
}

const tEncoderDriverOps MT6835_driver_ops = {
    .init = MT6835_init,
    .read_angle = MT6835_read_angle,
    .get_resolution = MT6835_get_resolution,
    .reset = MT6835_reset,
    .get_state = MT6835_get_state,
};

// ---- 句柄创建/销毁（资源注入点） ----

EncoderChipHandle MT6835_create(const tSpiBusIf *bus, const tTimeIf *time)
{
    if (!bus || !time)
        return NULL;

    tMT6835_ctx *ctx = (tMT6835_ctx *)calloc(1U, sizeof(tMT6835_ctx));
    if (!ctx)
        return NULL;

    ctx->dstate = DEV_OFFLINE;
    ctx->eng.bus = bus;
    ctx->eng.time = time;
    return (EncoderChipHandle)ctx;
}

void MT6835_destroy(EncoderChipHandle h)
{
    free(h);
}
