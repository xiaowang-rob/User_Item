/***************************************************************************************************
 * @file    rgb.c
 * @brief   RGB LED 驱动 (修正版 - 使用u32 DMA缓冲区)
 ***************************************************************************************************/

#include "bsp_led.h"
#include "gpio.h"
#include "config.h"

/* ========== GPIO LED ========== */
void BSP_LED_CanTogglePin(void)
{
    HAL_GPIO_TogglePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN);
}

void BSP_LED_EncoderTogglePin(void)
{
    HAL_GPIO_TogglePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN);
}
void BSP_LED_CanSetPin(bool on)
{
    HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
void BSP_LED_EncoderSetPin(bool on)
{
    HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
