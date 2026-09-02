// ============================================================
// hw_rgb_pwm.c — RGB(WS2812) PWM+DMA 推流接口表实现（xdr_p_o1.2 / TIM4）
//
// 把 TIM4-CH2 的 PWM-DMA 推流封装成 usr/if 的 tPwmDmaIf 契约。
// 厂商库符号与 HAL 中断回调只出现在本文件：
//   - HAL_TIM_PWM_PulseFinishedCallback 是本资源自持的中断入口，
//     推完一轮即复位 busy 并通知注册的回调
// ============================================================

#include "hw_rgb_pwm.h"

#include "hw_pinmap.h" // RGB_PWM_GET_HTIM、RGB_PWM_CHANNEL1、CODE_1/CODE_0
#include "tim.h"       // CubeMX: extern TIM_HandleTypeDef htim4
#include "stm32f4xx_hal.h"

typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    volatile bool busy;
    void (*done_cb)(void *arg);
    void *done_arg;
} tRgbPwmRes;

static tRgbPwmRes g_rgb_res = {
    .htim = &RGB_PWM_GET_HTIM,
    .channel = RGB_PWM_CHANNEL1,
    .busy = false,
};

// ---- HAL 中断转发（只出现在 hw） ----
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &RGB_PWM_GET_HTIM)
    {
        g_rgb_res.busy = false; // 本轮推完
        if (g_rgb_res.done_cb)
            g_rgb_res.done_cb(g_rgb_res.done_arg);
    }
}

// ---- tPwmDmaIf 实现 ----

static bool rgb_start_dma(void *ctx, const uint32_t *ccr_buf, uint16_t len)
{
    tRgbPwmRes *r = (tRgbPwmRes *)ctx;
    if (!r || !ccr_buf || len == 0U)
        return false;
    if (r->busy)
        return false; // 上一轮未完成，拒绝启动（调用方应查 is_busy）

    r->busy = true;
    if (HAL_TIM_PWM_Start_DMA(r->htim, r->channel, (uint32_t *)ccr_buf, len) != HAL_OK)
    {
        r->busy = false;
        return false;
    }
    return true;
}

static void rgb_stop_dma(void *ctx)
{
    tRgbPwmRes *r = (tRgbPwmRes *)ctx;
    if (!r)
        return;
    HAL_TIM_PWM_Stop_DMA(r->htim, r->channel);
    r->busy = false;
}

static bool rgb_is_busy(void *ctx)
{
    tRgbPwmRes *r = (tRgbPwmRes *)ctx;
    return r ? r->busy : false;
}

static void rgb_register_done(void *ctx, void (*cb)(void *arg), void *arg)
{
    tRgbPwmRes *r = (tRgbPwmRes *)ctx;
    if (!r)
        return;
    r->done_cb = cb;
    r->done_arg = arg;
}

static bool rgb_get_code_cfg(void *ctx, uint32_t *code_1, uint32_t *code_0)
{
    tRgbPwmRes *r = (tRgbPwmRes *)ctx;
    if (!r || !code_1 || !code_0)
        return false;
    *code_1 = CODE_1; // 逻辑 1 的 CCR 值（由该 PWM 分辨率决定，见 hw_pinmap）
    *code_0 = CODE_0;
    return true;
}

static const tPwmDmaIf g_rgb_pwm_if = {
    .ctx = &g_rgb_res,
    .start_dma = rgb_start_dma,
    .stop_dma = rgb_stop_dma,
    .is_busy = rgb_is_busy,
    .register_done = rgb_register_done,
    .get_code_cfg = rgb_get_code_cfg,
};

const tPwmDmaIf *hw_rgb_pwm_get(void)
{
    return &g_rgb_pwm_if;
}

uint8_t hw_rgb_pixel_num(void)
{
    return Pixel_NUM;
}
