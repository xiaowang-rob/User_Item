#include "adcDr.h"
#include "adc.h"
#include "tim.h"
#include "system_parameters.h"


#ifdef __DEBUG__
#include "math_fast.h"

u32 time_adc_zero = 0;
u32 time_adc_last = 0;
u32 time_adc_T = 0;
u32 time_adc_run = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        time_adc_run = (HAL_GetTick_us() - time_adc_last) * 2;
        //    time_adc_T = HAL_GetTick_us()-time_adc_zero;
        //    time_adc_zero = HAL_GetTick_us();
    }
}
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
        time_adc_last = HAL_GetTick_us();
}
#endif
u16 ADC1_buffer[3];
u8 ADC2_buffer[4];

#define ADCval_to_Cur rate_CurrentSample * 3.3f / 4095.0f
#define ADCval_to_Vol 3.3f * 16.0f / 255.0f
const u8 adc_to_temp[121] = {
    // ADC 69-78 -> 温度值
    120, 119, 118, 117, 116, 115, 114, 113, 112, 111,
    // ADC 79-88 -> 温度值
    110, 109, 108, 107, 106, 105, 104, 103, 102, 101,
    // ADC 89-98 -> 温度值
    100, 99, 98, 97, 96, 95, 94, 93, 92, 91,
    // ADC 99-108 -> 温度值
    90, 89, 88, 87, 86, 85, 84, 83, 82, 81,
    // ADC 109-118 -> 温度值
    80, 79, 78, 77, 76, 75, 74, 73, 72, 71,
    // ADC 119-128 -> 温度值
    70, 69, 68, 67, 66, 65, 64, 63, 62, 61,
    // ADC 129-138 -> 温度值
    60, 59, 58, 57, 56, 55, 54, 53, 52, 51,
    // ADC 139-148 -> 温度值
    50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
    // ADC 149-158 -> 温度值
    40, 39, 38, 37, 36, 35, 34, 33, 32, 31,
    // ADC 159-168 -> 温度值
    30, 29, 28, 27, 26, 25, 24, 23, 22, 21,
    // ADC 169-178 -> 温度值
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11,
    // ADC 179-188 -> 温度值
    10, 9, 8, 7, 6, 5, 4, 3, 2, 1,
    // ADC 189 -> 温度值
    0};

void ADC_DR_Init()
{
    HAL_TIM_Base_Start_IT(&htim8);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (u32 *)ADC1_buffer, 3);
    HAL_ADC_Start_DMA(&hadc2, (u32 *)ADC2_buffer, 4);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_4, 1);
}
void ADC1_sample()
{

    HAL_ADC_Start_DMA(&hadc1, (u32 *)ADC1_buffer, 3);
}
void ADC2_sample()
{
    HAL_ADC_Start_DMA(&hadc2, (u32 *)ADC2_buffer, 4);
}
void ADC_GET_Current(float *ui, float *vi, float *wi)
{
    *ui = (float)ADC1_buffer[0] * ADCval_to_Cur;
    *vi = (float)ADC1_buffer[1] * ADCval_to_Cur;
    *wi = (float)ADC1_buffer[2] * ADCval_to_Cur;
}
void ADC_sample_change(u16 compare)
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_4, compare);
}
void ADC_GET_Voltage(float *voltage)
{
    *voltage = ADC2_buffer[3] * ADCval_to_Vol;
}
u8 tempIndex = 0;
void ADC_GET_Temp(u8 *ut, u8 *vt, u8 *wt, float *Temperature)
{
    switch (tempIndex)
    {
    case 0:
        *ut = adc_to_temp[ADC2_buffer[0] - temp0_adc_val];
        tempIndex = 1;
        break;
    case 1:
        *vt = adc_to_temp[ADC2_buffer[1] - temp0_adc_val];
        tempIndex = 2;
        break;
    case 2:
        *wt = adc_to_temp[ADC2_buffer[2] - temp0_adc_val];
        tempIndex = 3;
        break;
    default:
        *Temperature = (float)(*ut + *vt + *wt) / 3.0f;
        tempIndex = 0;
        break;
    }
}