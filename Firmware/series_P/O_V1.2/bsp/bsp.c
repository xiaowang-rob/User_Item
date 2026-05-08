/**
 * @file    bsp.c
 * @brief   BSP实现 - 板级支持包
 * @note    薄封装，直接调用CubeMX生成的HAL函数
 */

#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

#include "usbd_cdc_if.h"
#include "stm32f4xx_it.h"
#include "bsp.h"
#include "config.h"

void BSP_enable_irq()
{
    __enable_irq();
}
void BSP_disable_irq()
{
    __disable_irq();
}
// 中断向量表偏移
void BSP_SetVectorTableOffset(u32 offset)
{
    SCB->VTOR = FLASH_BASE | offset;
}

u32 BSP_GetTick(void)
{
    return HAL_GetTick();
}

u32 BSP_GetTick_us(void)
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

void BSP_Delay(u32 ms)
{
    HAL_Delay(ms);
}

void BSP_SystemReset(void)
{
    NVIC_SystemReset();
}
