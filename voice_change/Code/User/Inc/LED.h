#include "main.h"

#define blue_set() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define blue_reset() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)

void LED_Init();
void RED_pwm(uint8_t compare);
void LED_run();
void LED_mode_change(uint8_t mode);