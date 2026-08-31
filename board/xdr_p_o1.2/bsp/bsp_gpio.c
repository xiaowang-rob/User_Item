#include "bsp_gpio.h"
#include "gpio.h"
#include "board_config.h"

// ========== GPIO LED ==========
void bsp_led_0_toggle_pin(void)
{
    HAL_GPIO_TogglePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN);
}
void bsp_led_0_set_pin(bool on)
{
    HAL_GPIO_WritePin(LED_CANrx_GPIOx, LED_CANrx_GPIOx_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void bsp_led_1_toggle_pin(void)
{
    HAL_GPIO_TogglePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN);
}

void bsp_led_1_set_pin(bool on)
{
    HAL_GPIO_WritePin(LED_ENCODER_GPIOx, LED_ENCODER_GPIOx_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
