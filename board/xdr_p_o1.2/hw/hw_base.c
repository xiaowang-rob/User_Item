// ============================================================
// hw_base.c — 板级基础服务实现（xdr_p_o1.2 / STM32F405）
//
// 本文件是 hw 层的第一个接口表实现：把 CMSIS/HAL 的时间能力封装成
// usr/if 的 tTimeIf 契约。厂商库符号只出现在这里，usr 侧无感知。
//
// - get_ms：HAL_GetTick（SysTick 1ms，CubeMX 环境已就绪）
// - get_us：DWT->CYCCNT 周期计数 / 主频（1us@168MHz = 168 周期）
// ============================================================

#include "hw_base.h"

#include "stm32f4xx_hal.h" // HAL_GetTick；并引入 CoreDebug/DWT/SystemCoreClock

// ---- 时间基准：tTimeIf 实现 ----

static uint32_t hw_get_ms(void *ctx)
{
    (void)ctx;
    return HAL_GetTick();
}

static uint32_t hw_get_us(void *ctx)
{
    (void)ctx;
    // 先快照再除，避免多次读 CYCCNT 造成不一致
    uint32_t cyccnt = DWT->CYCCNT;
    return cyccnt / (SystemCoreClock / 1000000U);
}

static const tTimeIf g_time_if = {
    .ctx = NULL,        // 无实例状态，实现使用全局/内核资源
    .get_ms = hw_get_ms,
    .get_us = hw_get_us,
};

// 使能 DWT 周期计数器（微秒基准的前提）。
// 调用时机：HAL_Init() 之后、任何业务启动之前，由组装层/启动路径调用一次。
void hw_base_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

const tTimeIf *hw_time_get(void)
{
    return &g_time_if;
}
