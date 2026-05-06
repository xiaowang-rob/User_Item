#include "bsp_pwm.h"
#include "tim.h"
#include "config.h"

__weak void BSP_FOC_ITCallback(void)
{
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
        if (TIM8->CR1 & TIM_CR1_DIR)
        {
            // ========== 上溢中断 ==========
            return;
        }
        else
        {
            // ========== 下溢中断 ==========
            BSP_FOC_ITCallback();
            return;
        }
    }
}

/* ============================================
 * GPIO - 通用输入输出映射
 * ============================================ */
void BSP_POWER_12V_Control(bool on)
{
    if (on)
    {
        HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_RESET);
    }
}

/* ============================================
 * PWM - 电机驱动
 * ============================================ */
void BSP_PWM_SetCompare(u16 ticA, u16 ticB, u16 ticC)
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, ticA);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, ticB);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_3, ticC);
}
void BSP_PWM_Enable(void)
{
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_3);
}
void BSP_PWM_Disable(void)
{
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_3);
}