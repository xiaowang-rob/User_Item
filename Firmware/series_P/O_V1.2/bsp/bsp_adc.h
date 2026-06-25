#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "bsp.h"

void bsp_adc_init(void);

void bsp_adc_get_voltage_temp(float *voltage, float *temperature);

void bsp_adc_sample_change(u16 compare);
void bsp_adc_get_current(float *ui, float *vi, float *wi);
bool bsp_adc_calibrate_current(float *ui_offset, float *vi_offset, float *wi_offset);
void bsp_set_adc_current_offset(float ui_offset, float vi_offset, float wi_offset);
void bsp_adc_idle_track(void);

#endif // __BSP_ADC_H