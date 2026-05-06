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
    /* 计算微秒级时间戳 */
    u32 tick_ms = BSP_GetTick();                                                        // 获取当前毫秒级系统时间
    u32 tick_us = (tick_ms * 1000) + (DWT->CYCCNT / (HAL_RCC_GetHCLKFreq() / 1000000)); // 转换为微秒
    return tick_us;
}

void BSP_Delay(u32 ms)
{
    HAL_Delay(ms);
}

void BSP_SystemReset(void)
{
    NVIC_SystemReset();
}
