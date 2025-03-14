#include "LED.h"
#include "tim.h"

uint8_t LED_MODE = 0;
void LED_Init()
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 100);
}
void RED_pwm(uint8_t compare)
{
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, compare);
}
void LED_run()
{
    switch (LED_MODE)
    {
    case 0:
        for (int i = 0; i < 100; i++)
        {
            RED_pwm(i);
            HAL_Delay(10);
        }
        blue_set();
        HAL_Delay(100);
        for (int i = 100; i > 0; i--)
        {
            RED_pwm(i);
            HAL_Delay(10);
        }
        blue_reset();
        HAL_Delay(100);
        break;
    case 1:
        RED_pwm(100);
        HAL_Delay(100);
        RED_pwm(0);
        HAL_Delay(100);
        break;

    default:
        break;
    }
}
void LED_mode_change(uint8_t mode)
{
    LED_MODE = mode;
}