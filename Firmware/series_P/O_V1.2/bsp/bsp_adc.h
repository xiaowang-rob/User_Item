#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "bsp.h"

void BSP_AdcInit(void);
void BSP_AdcSampleChange(u16 compare);
void BSP_TempVbusSample(void);
void BSP_AdcGetCurrent(float *iu, float *iv, float *iw);
void BSP_SampleCurrent2Shunt(u8 sector, float *ialpha, float *ibeta);
bool BSP_AdcCalibrateCurrent(float *ui_offset, float *vi_offset, float *wi_offset);
void BSP_SetAdcCurrentOffset(float ui_offset, float vi_offset, float wi_offset);
void BSP_AdcIdleTrack(void);
void BSP_AdcGetVoltage(float *voltage);
void BSP_AdcGetTemp(float *temperature);

#endif /* __BSP_ADC_H */