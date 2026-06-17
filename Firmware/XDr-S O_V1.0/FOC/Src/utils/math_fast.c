#include "math_fast.h"

/**
 * @brief 获取微秒级系统时间戳
 * @return 当前时间（微秒）
 */
u32 HAL_GetTick_us(void)
{
    // 获取当前ms
    u32 m = HAL_GetTick();
    // 获取嘀嗒定时器重装载值
    const u32 tms = SysTick->LOAD + 1;
    // 获取当前滴答定时器计数值
    __IO u32 u = tms - SysTick->VAL;
    // 返回对应的值
    return (m * 1000 + (u * 1000) / tms);
}
