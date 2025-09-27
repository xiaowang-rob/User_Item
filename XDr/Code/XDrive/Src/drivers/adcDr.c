#include "adcDr.h"
#include "adc.h"
#include "tim.h"
#include "base_parameters.h"

u16 ADC1_buffer[3];
u8 ADC2_buffer[4];

#define ADCval_to_Cur rate_CurrentSample * 3.3f / 4095.0f
#define ADCval_to_Vol 3.3f * 16.0f / 255.0f

void ADC_DR_Init(void)
{
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (u32 *)ADC1_buffer, 3);
    HAL_ADC_Start_DMA(&hadc2, (u32 *)ADC2_buffer, 4);
}
void ADC_GET_Current(float *ui, float *vi, float *wi)
{
    *ui = (float)ADC1_buffer[0] * ADCval_to_Cur;
    *vi = (float)ADC1_buffer[1] * ADCval_to_Cur;
    *wi = (float)ADC1_buffer[2] * ADCval_to_Cur;
}
void ADC_slamp_change(u16 compare)
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_4, compare);
}
void ADC_GET_Voltage(float *Udc)
{
    *Udc=(float)ADC2_buffer[3] * ADCval_to_Vol;
}
u8 tempIndex = 0;
void ADC_GET_Temp(u8 *ut, u8 *vt, u8 *wt, u8 *Temperature)
{
    u8 temp;
    switch (tempIndex)
    {
    case 0:
        for (temp = 0; ADC2_buffer[0] <= temp_to_adc[temp]; temp++)
            ;
        *ut = temp;
        tempIndex = 1;
        break;
    case 1:
        for (temp = 0; ADC2_buffer[0] <= temp_to_adc[temp]; temp++)
            ;
        *vt = temp;
        tempIndex = 2;
        break;
    case 2:
        for (temp = 0; ADC2_buffer[0] <= temp_to_adc[temp]; temp++)
            ;
        *wt = temp;
        tempIndex = 3;
        break;
    default:
        *Temperature = (float)(*ut + *vt + *wt) / 3.0f;
        tempIndex = 0;
        break;
    }
}