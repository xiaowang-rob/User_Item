// BSP实现 - 板级支持包
// 薄封装，直接调用CubeMX生成的HAL函数

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
#include "bsp_base.h"
#include "board_config.h"

void bsp_enable_irq()
{
    __enable_irq();
}
void bsp_disable_irq()
{
    __disable_irq();
}
// 中断向量表偏移
void bsp_set_vector_table_offset(u32 offset)
{
    SCB->VTOR = FLASH_BASE | offset;
}

u32 bsp_get_tick(void)
{
    return HAL_GetTick();
}

u32 bsp_get_tick_us(void)
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

void bsp_delay(u32 ms)
{
    HAL_Delay(ms);
}

void bsp_system_reset(void)
{
    NVIC_SystemReset();
}
