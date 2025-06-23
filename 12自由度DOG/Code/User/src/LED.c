#include "LED.h"
uint16_t pulse[104] = {
													0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
													0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,\
	                       69,69,69,69,69,69,69,69,\
                         69,69,69,69,69,69,69,69,\
                         142,142,142,142,142,142,142,142,
	};

void LED_free()
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
//   osDelay(1000);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
//   osDelay(1000);
    HAL_Delay(1000);
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    HAL_TIM_PWM_Stop_DMA(&htim1,TIM_CHANNEL_3);
	 HAL_TIM_PWM_Stop_DMA(&htim1,TIM_CHANNEL_2);
}
void LGB_mode_1()
{
   HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_3,(uint32_t *)pulse,(104)); 
 HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_2,(uint32_t *)pulse,(104)); 
}