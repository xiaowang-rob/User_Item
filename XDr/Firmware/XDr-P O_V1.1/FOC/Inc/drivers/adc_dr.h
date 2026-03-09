#ifndef __ADC_DR_H
#define __ADC_DR_H

#include "main.h"

/* 函数声明 */
void fAdcDrInit(void);
void fAdc2Sample(void);
void fAdcGetCurrent(float *ui, float *vi, float *wi);
void fAdcSampleChange(u16 compare);
void fAdcGetVoltage(float *voltage);
void fAdcGetTemp(u8 *ut, u8 *vt, u8 *wt, float *temperature);

#endif /* __ADC_DR_H */