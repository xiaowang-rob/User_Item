#include "adcDr.h"
#include "adc.h"
#include "tim.h"
#include "system_parameters.h"
#include "filter.h"
static bool _calibration = false;

static float Cur_zero_u = 0;
static float Cur_zero_v = 0;
static float Cur_zero_w = 0;

static u8 _tic;
static float _u_sum = 0;
static float _v_sum = 0;
static float _w_sum = 0;

static float Cur_u;
static float Cur_v;
static float Cur_w;

u16 ADC1_buffer[3];
u8 ADC2_buffer[4];

#define ADCval_to_Cur rate_CurrentSample * 3.3f / 4095.0f
#define ADCval_to_Vol 3.3f * 16.0f / 256.0f
u8 adc2_index = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {

        if (!_calibration)
        {
            _u_sum += (float)ADCval_to_Cur * ADC1_buffer[0];
            _v_sum += (float)ADCval_to_Cur * ADC1_buffer[1];
            _w_sum += (float)ADCval_to_Cur * ADC1_buffer[2];
            _tic++;
            if (_tic == 100)
            {
                Cur_zero_u = _u_sum / 100;
                Cur_zero_v = _v_sum / 100;
                Cur_zero_w = _w_sum / 100;
                _calibration = true;
            }
        }
    }
}

const u8 adc_to_temp[255] = {
    125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125,
    125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125,
    125, 125, 125, 120, 118, 115, 112, 110, 108, 107, 105, 102, 100, 99, 98, 96,
    95, 93, 92, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78,
    78, 77, 76, 75, 74, 73, 72, 72, 71, 70, 69, 69, 68, 68, 67, 66,
    66, 65, 64, 64, 63, 63, 62, 62, 61, 61, 60, 60, 59, 59, 58, 58,
    57, 57, 56, 56, 55, 55, 55, 54, 54, 53, 53, 53, 52, 52, 52, 51,
    51, 50, 50, 50, 49, 49, 48, 48, 48, 47, 47, 47, 46, 46, 45, 45,
    45, 44, 44, 44, 43, 43, 43, 42, 42, 42, 41, 41, 41, 40, 40, 40,
    39, 39, 39, 38, 38, 38, 38, 37, 37, 37, 36, 36, 36, 35, 35, 35,
    34, 34, 34, 34, 33, 33, 33, 32, 32, 32, 31, 31, 31, 31, 30, 30,
    30, 29, 29, 29, 29, 28, 28, 28, 28, 27, 27, 27, 26, 26, 26, 26,
    25, 25, 25, 24, 24, 24, 24, 23, 23, 23, 22, 22, 22, 22, 21, 21,
    21, 21, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 18, 17, 17, 17,
    17, 16, 16, 16, 16, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 12,
    12, 12, 12, 11, 11, 11, 11, 10, 10, 9, 9, 8, 7, 6, 6, 5};
void ADC_Cur_Calibration()
{
    _tic = 0;
    _u_sum = 0;
    _v_sum = 0;
    _w_sum = 0;
    _calibration = false;
}
#define filter_buffer_size 5

PulseInterferenceFilter ui_filter;
PulseInterferenceFilter vi_filter;
PulseInterferenceFilter wi_filter;
static u16 ui_buffer[filter_buffer_size];
static u16 vi_buffer[filter_buffer_size];
static u16 wi_buffer[filter_buffer_size];

void ADC_DR_Init()
{
    HAL_TIM_Base_Start_IT(&htim8);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (u32 *)ADC1_buffer, 3);
    HAL_ADC_Start_DMA(&hadc2, (u32 *)ADC2_buffer, 4);
    ADC_sample_change(2050);
    pulse_interference_init(&ui_filter, ui_buffer, filter_buffer_size);
    pulse_interference_init(&vi_filter, vi_buffer, filter_buffer_size);
    pulse_interference_init(&wi_filter, wi_buffer, filter_buffer_size);
}

static u16 _tic_smp = 0;
void ADC2_sample()
{
    _tic_smp++;
    if (_tic_smp < TEMP_sample_T)
        return;
    _tic_smp = 0;
    HAL_ADC_Start_DMA(&hadc2, (u32 *)ADC2_buffer, 4);
}
void ADC_GET_Current(float *ui, float *vi, float *wi)
{
    ADC1_buffer[0] = pulse_interference_filter(&ui_filter, ADC1_buffer[0]);
    ADC1_buffer[1] = pulse_interference_filter(&vi_filter, ADC1_buffer[1]);
    ADC1_buffer[2] = pulse_interference_filter(&wi_filter, ADC1_buffer[2]);
    Cur_u = (float)(ADC1_buffer[0] * ADCval_to_Cur - Cur_zero_u) > 0 ? (float)(ADC1_buffer[0] * ADCval_to_Cur - Cur_zero_u) : 0;
    Cur_v = (float)(ADC1_buffer[1] * ADCval_to_Cur - Cur_zero_v) > 0 ? (float)(ADC1_buffer[1] * ADCval_to_Cur - Cur_zero_v) : 0;
    Cur_w = (float)(ADC1_buffer[2] * ADCval_to_Cur - Cur_zero_w) > 0 ? (float)(ADC1_buffer[2] * ADCval_to_Cur - Cur_zero_w) : 0;

    *ui = Cur_u;
    *vi = Cur_v;
    *wi = Cur_w;
    //    *ui = first_order_lag_filter(&ui_filter, Cur_u);
    //    *vi = first_order_lag_filter(&vi_filter, Cur_v);
    //    *wi = first_order_lag_filter(&wi_filter, Cur_w);
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
        *ut = adc_to_temp[ADC2_buffer[0]];
        tempIndex = 1;
        break;
    case 1:
        *vt = adc_to_temp[ADC2_buffer[1]];
        tempIndex = 2;
        break;
    case 2:
        *wt = adc_to_temp[ADC2_buffer[2]];
        tempIndex = 3;
        break;
    default:
        *Temperature = (float)(*ut + *vt + *wt) / 3.0f;
        tempIndex = 0;
        break;
    }
}