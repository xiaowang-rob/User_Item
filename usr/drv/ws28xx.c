// ============================================================
// ws28xx.c — WS2812/WS28xx RGB 灯驱动（usr/drv）
//
// 芯片协议：单线串行、每灯 24bit（GRB 序）、每位由"高占空比(1)/低占空比(0)"
// 的定时脉冲表示，整体由 PWM+DMA 连续推 CCR 值序列产生。
// 驱动职责：颜色+亮度 → 逐位 CCR 编码 → 交给 tPwmDmaIf 异步推送。
//
// 仅依赖 usr/if 接口表（tPwmDmaIf），无任何板级/厂商库依赖。
// ============================================================

#include <stdlib.h>
#include <string.h>

#include "usr/abs/led.h"

#include "rgb_drivers.h"

#define WS_RESET_BITS 100U // 帧尾复位（低电平）位数

typedef struct
{
    const tPwmDmaIf *pwm; // 注入：PWM-DMA 推流能力
    uint8_t num_pixels;   // 灯珠数（组装层按板上实际传入）
    uint32_t buf_len;     // = num*24 + reset

    uint32_t code_1; // 逻辑 1 的 CCR 值
    uint32_t code_0; // 逻辑 0 的 CCR 值

    tRGBColor color;      // 当前颜色
    uint8_t brightness;   // 整体亮度 0~255
    bool inited;

    uint32_t *ccr_buf; // 编码缓冲（动态分配）
} tWs28xx_ctx;

// 亮度缩放：v ∈ 0..255，b ∈ 0..255 → 0..255
static inline uint8_t scale_brightness(uint8_t v, uint8_t b)
{
    return (uint8_t)(((uint16_t)v * b + 127U) / 255U);
}

// 把颜色+亮度编码成 CCR 序列（GRB 顺序、MSB first）
static void ws28xx_build_stream(tWs28xx_ctx *ctx)
{
    uint32_t idx = 0U;
    for (uint8_t pixel = 0U; pixel < ctx->num_pixels; pixel++)
    {
        // 通道顺序：G, R, B（WS2812 数据格式）
        uint8_t ch[3];
        ch[0] = scale_brightness(ctx->color.G, ctx->brightness);
        ch[1] = scale_brightness(ctx->color.R, ctx->brightness);
        ch[2] = scale_brightness(ctx->color.B, ctx->brightness);

        for (uint8_t c = 0U; c < 3U; c++)
        {
            for (int bit = 7; bit >= 0; bit--)
            {
                ctx->ccr_buf[idx++] = ((ch[c] >> bit) & 0x01U) ? ctx->code_1 : ctx->code_0;
            }
        }
    }
    // 帧尾复位：全部拉低足够时长
    for (uint32_t i = idx; i < ctx->buf_len; i++)
        ctx->ccr_buf[i] = 0U;
}

// ---- ops 实现 ----

static bool ws28xx_init(RgbHandle h)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)h;
    if (!ctx)
        return false;

    if (!ctx->pwm->get_code_cfg(ctx->pwm->ctx, &ctx->code_1, &ctx->code_0))
        return false;

    // 上电默认全灭
    memset(ctx->ccr_buf, 0, ctx->buf_len * sizeof(uint32_t));
    ctx->brightness = 255U;
    ctx->color.R = 0U;
    ctx->color.G = 0U;
    ctx->color.B = 0U;
    ctx->inited = true;
    return true;
}

static void ws28xx_set_rgb(RgbHandle h, tRGBColor color)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)h;
    if (!ctx)
        return;
    ctx->color = color;
}

static void ws28xx_set_brightness(RgbHandle h, uint8_t brightness)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)h;
    if (!ctx)
        return;
    ctx->brightness = brightness;
}

static void ws28xx_refresh(RgbHandle h)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)h;
    if (!ctx || !ctx->inited)
        return;

    if (ctx->pwm->is_busy(ctx->pwm->ctx))
        return; // 上一帧仍在推，丢弃本次（下次 refresh 再更新）

    ws28xx_build_stream(ctx);
    ctx->pwm->start_dma(ctx->pwm->ctx, ctx->ccr_buf, (uint16_t)ctx->buf_len);
}

const tRgbDriverOps ws28xx_driver_ops = {
    .init = ws28xx_init,
    .set_rgb = ws28xx_set_rgb,
    .set_brightness = ws28xx_set_brightness,
    .refresh = ws28xx_refresh,
};

// ---- 句柄创建/销毁（资源注入点） ----

RgbHandle ws28xx_create(const tPwmDmaIf *pwm, uint8_t num_pixels)
{
    if (!pwm || num_pixels == 0U)
        return NULL;

    tWs28xx_ctx *ctx = (tWs28xx_ctx *)calloc(1U, sizeof(tWs28xx_ctx));
    if (!ctx)
        return NULL;

    ctx->buf_len = (uint32_t)num_pixels * 24U + WS_RESET_BITS;
    ctx->ccr_buf = (uint32_t *)calloc(ctx->buf_len, sizeof(uint32_t));
    if (!ctx->ccr_buf)
    {
        free(ctx);
        return NULL;
    }

    ctx->pwm = pwm;
    ctx->num_pixels = num_pixels;
    ctx->brightness = 255U;
    return (RgbHandle)ctx;
}

void ws28xx_destroy(RgbHandle h)
{
    tWs28xx_ctx *ctx = (tWs28xx_ctx *)h;
    if (!ctx)
        return;
    free(ctx->ccr_buf);
    free(ctx);
}
