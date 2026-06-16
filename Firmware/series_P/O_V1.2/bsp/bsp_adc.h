#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "bsp.h"

void BSP_AdcInit(void);

void BSP_AdcGetVoltage_Temp(float *voltage, float *temperature);

void BSP_AdcSampleChange(u16 compare);
void BSP_AdcGetCurrent(float *ui, float *vi, float *wi);
bool BSP_AdcCalibrateCurrent(float *ui_offset, float *vi_offset, float *wi_offset);
void BSP_SetAdcCurrentOffset(float ui_offset, float vi_offset, float wi_offset);
void BSP_AdcIdleTrack(void);

#endif /* __BSP_ADC_H */