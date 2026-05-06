#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "bsp.h"

void BSP_AdcInit(void);
void BSP_AdcSampleChange(u16 compare);
void BSP_TempVbusSample(void);
void BSP_AdcGetCurrent(float *iu, float *iv, float *iw);
void BSP_AdcRecalibrateCurrent(void);
bool BSP_AdcRecalibrateDone(void);
void BSP_AdcGetVoltage(float *voltage);
void BSP_AdcGetTemp(float *temperature);

#endif /* __BSP_ADC_H */