// ============================================================
// as5047.c — AS5047 编码器驱动（usr/drv，同步读角）
//
// 芯片协议：SPI Mode1(CPOL=0,CPHA=1) / 16bit / 14bit，分辨率 16384
// 读角序列（一次 CS 周期两段）：
//   段1 发送读命令 0x7FFF（响应丢弃）
//   段2 发送 NOP 0x0000，同时移出角度帧
// 角度帧：bit14 = 错误标志；bit13..0 = 角度
//
// 仅依赖 usr/if 接口表与 enc_spi_engine，无任何板级/厂商库依赖。
// ============================================================

#include <stdlib.h>

#include "usr/abs/device.h"
#include "usr/abs/encoder.h"

#include "enc_spi_engine.h"
#include "encoder_drivers.h"

#define AS5047_RESOLUTION 16384U
#define AS5047_CMD_READ 0x7FFFU // 读角度命令
#define AS5047_CMD_NOP 0x0000U  // NOP（用于移出角度数据）
#define AS5047_ERR_FLAG 0x4000U // bit14 错误标志
#define AS5047_ANGLE_MASK 0x3FFFU

typedef struct
{
    eDeviceStatus dstate;
    tEncSpiEngine eng;

    uint16_t cmd_read; // 段1 tx（16bit 值的内存视图）
    uint16_t cmd_nop;  // 段2 tx
    uint8_t rx1[2];    // 段1 rx（丢弃）
    uint8_t rx2[2];    // 段2 rx（角度帧，LE half-word 视图）

    uint16_t raw;  // 最近一次有效角度
    uint32_t ts;   // 最近一次有效时间戳(ms)
} tAS5047_ctx;

// ---- ops 实现 ----

static bool AS5047_init(EncoderChipHandle h)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)h;
    if (!ctx)
        return false;

    // SPI Mode1(CPOL=0,CPHA=1)/16bit —— 芯片协议事实，由驱动声明、bus 执行
    if (!ctx->eng.bus->set_mode(ctx->eng.bus->ctx, 0U, 1U, 16U))
        return false;

    ctx->cmd_read = AS5047_CMD_READ;
    ctx->cmd_nop = AS5047_CMD_NOP;
    ctx->raw = 0U;
    ctx->ts = 0U;
    ctx->dstate = DEV_ONLINE;
    return true;
}

static bool AS5047_read_angle(EncoderChipHandle h, uint16_t *raw, uint32_t *ts_ms)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)h;
    if (!ctx || !raw || !ts_ms)
        return false;

    tEncXferSeg segs[2];
    segs[0].tx = (const uint8_t *)&ctx->cmd_read; // LE half-word 视图
    segs[0].rx = ctx->rx1;
    segs[0].len = 2U;
    segs[1].tx = (const uint8_t *)&ctx->cmd_nop;
    segs[1].rx = ctx->rx2;
    segs[1].len = 2U;

    if (!enc_engine_run(&ctx->eng, segs, 2U))
    {
        ctx->dstate = DEV_RUN_ERROR;
        return false;
    }

    uint16_t frame = (uint16_t)(ctx->rx2[0] | ((uint16_t)ctx->rx2[1] << 8));
    if (frame & AS5047_ERR_FLAG) // 芯片报告错误
    {
        ctx->dstate = DEV_RUN_ERROR;
        return false;
    }

    ctx->raw = frame & AS5047_ANGLE_MASK;
    ctx->ts = ctx->eng.time->get_ms(ctx->eng.time->ctx);
    ctx->dstate = DEV_RUNNING;
    *raw = ctx->raw;
    *ts_ms = ctx->ts;
    return true;
}

static bool AS5047_get_resolution(EncoderChipHandle h, uint16_t *res)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)h;
    if (!ctx || !res)
        return false;
    *res = AS5047_RESOLUTION;
    return true;
}

static void AS5047_reset(EncoderChipHandle h)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)h;
    if (!ctx)
        return;
    enc_engine_abort(&ctx->eng);
    ctx->raw = 0U;
    ctx->ts = 0U;
    ctx->dstate = DEV_ONLINE;
}

static uint8_t AS5047_get_state(EncoderChipHandle h)
{
    tAS5047_ctx *ctx = (tAS5047_ctx *)h;
    return (uint8_t)(ctx ? ctx->dstate : DEV_OFFLINE);
}

const tEncoderDriverOps AS5047_driver_ops = {
    .init = AS5047_init,
    .read_angle = AS5047_read_angle,
    .get_resolution = AS5047_get_resolution,
    .reset = AS5047_reset,
    .get_state = AS5047_get_state,
};

// ---- 句柄创建/销毁（资源注入点） ----

EncoderChipHandle AS5047_create(const tSpiBusIf *bus, const tTimeIf *time)
{
    if (!bus || !time)
        return NULL;

    tAS5047_ctx *ctx = (tAS5047_ctx *)calloc(1U, sizeof(tAS5047_ctx));
    if (!ctx)
        return NULL;

    ctx->dstate = DEV_OFFLINE;
    ctx->eng.bus = bus;
    ctx->eng.time = time;
    return (EncoderChipHandle)ctx;
}

void AS5047_destroy(EncoderChipHandle h)
{
    free(h);
}
