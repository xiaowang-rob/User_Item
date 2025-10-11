/*电流、电压、温度，mcu自带ADC采样*/
#ifndef ADC_DR_H
#define ADC_DR_H
#include "main.h"

// ADC值到温度的映射表
#define temp0_adc_val 69 // 0°对应ADC值

void ADC_DR_Init(void);
void ADC_GET_Current(float *ui, float *vi, float *wi);
void ADC_sample_change(u16 compare);
float ADC_GET_Voltage();
void ADC_GET_Temp(u8 *ut, u8 *vt, u8 *wt, u8 *Temperature);

#endif // ADC_DR_Hs