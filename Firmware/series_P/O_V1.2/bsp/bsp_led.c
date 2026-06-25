// ***************************************************************************************************
// RGB LED 驱动 (修正版 - 使用u32 DMA缓冲区)
// ***************************************************************************************************

#include "bsp_led.h"
#include "gpio.h"
#include "config.h"

// ========== GPIO LED ==========
void bsp_led_can_toggle_pin(void)
{
    HAL_GPIO_TogglePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN);
}

void bsp_led_encoder_toggle_pin(void)
{
    HAL_GPIO_TogglePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN);
}
void bsp_led_can_set_pin(bool on)
{
    HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
void bsp_led_encoder_set_pin(bool on)
{
    HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
