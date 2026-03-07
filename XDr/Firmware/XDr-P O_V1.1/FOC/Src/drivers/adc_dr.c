#include "adc_dr.h"
#include "adc.h"
#include "tim.h"
#include "drive_parameters.h"
#include "filter.h"

/* ADC相关配置定义 */

#define FILTER_BUFFER_SIZE 5

static const float ADC_VAL_TO_CUR_FACTOR = 3.3f * rate_CurrentSample / 4095.0f;
static const float ADC_VAL_TO_VOL_FACTOR = 3.3f * rate_VoltageSample / 255.0f;
/* ADC采样相关变量 */
static bool g_calibration_flag = false;

static float g_cur_zero_u = 0;
static float g_cur_zero_v = 0;
static float g_cur_zero_w = 0;

static u8 g_sample_counter = 0;
static float g_u_sum = 0;
static float g_v_sum = 0;
static float g_w_sum = 0;

static float g_cur_u = 0;
static float g_cur_v = 0;
static float g_cur_w = 0;

static u16 g_adc1_buffer[3];
static u8 g_adc2_buffer[4];

static u8 g_adc2_index = 0;

/* 滤波器实例 */
static tPulseInterferenceFilter g_ui_filter;
static tPulseInterferenceFilter g_vi_filter;
static tPulseInterferenceFilter g_wi_filter;

/* 滤波器缓冲区 */
static u16 g_ui_buffer[FILTER_BUFFER_SIZE];
static u16 g_vi_buffer[FILTER_BUFFER_SIZE];
static u16 g_wi_buffer[FILTER_BUFFER_SIZE];

/* 温度转换查找表 */
static const u8 g_adc_to_temp[255] = {
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
    16, 16, 16, 16, 15, 15, 15, 14, 14, 14, 14, 13, 13, 13, 12,
    12, 12, 12, 11, 11, 11, 11, 10, 10, 9, 9, 8, 7, 6, 6, 5};

static u32 sample_time_prev_ms = 0;
static u8 g_temp_index = 0;

/**
 * @brief ADC转换完成回调函数
 * @param hadc ADC句柄指针
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        if (!g_calibration_flag)
        {
            g_u_sum += (float)ADC_VAL_TO_CUR_FACTOR * g_adc1_buffer[0];
            g_v_sum += (float)ADC_VAL_TO_CUR_FACTOR * g_adc1_buffer[1];
            g_w_sum += (float)ADC_VAL_TO_CUR_FACTOR * g_adc1_buffer[2];
            g_sample_counter++;
            if (g_sample_counter == 100)
            {
                g_cur_zero_u = g_u_sum / 100;
                g_cur_zero_v = g_v_sum / 100;
                g_cur_zero_w = g_w_sum / 100;
                g_calibration_flag = true;
            }
        }
    }
}

/**
 * @brief 电流校准初始化
 */
void fAdcCurCalibration(void)
{
    g_sample_counter = 0;
    g_u_sum = 0;
    g_v_sum = 0;
    g_w_sum = 0;
    g_calibration_flag = false;
}

/**
 * @brief ADC数据采集初始化
 */
void fAdcDrInit(void)
{
    HAL_TIM_Base_Start_IT(&htim8);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (u32 *)g_adc1_buffer, 3);
    HAL_ADC_Start_DMA(&hadc2, (u32 *)g_adc2_buffer, 4);
    fAdcSampleChange(2050);
    fPulseInterferenceInit(&g_ui_filter, g_ui_buffer, FILTER_BUFFER_SIZE);
    fPulseInterferenceInit(&g_vi_filter, g_vi_buffer, FILTER_BUFFER_SIZE);
    fPulseInterferenceInit(&g_wi_filter, g_wi_buffer, FILTER_BUFFER_SIZE);
}

/**
 * @brief ADC2采样触发
 */
void fAdc2Sample(void)
{
    if (HAL_GetTick() - sample_time_prev_ms < TEMP_VBUS_TS_MS)
        return;
    sample_time_prev_ms = HAL_GetTick();
    HAL_ADC_Start_DMA(&hadc2, (u32 *)g_adc2_buffer, 4);
}

/**
 * @brief 获取电流值
 * @param ui U相电流指针
 * @param vi V相电流指针
 * @param wi W相电流指针
 */
void fAdcGetCurrent(float *ui, float *vi, float *wi)
{
    g_adc1_buffer[0] = fPulseInterferenceFilter(&g_ui_filter, g_adc1_buffer[0]);
    g_adc1_buffer[1] = fPulseInterferenceFilter(&g_vi_filter, g_adc1_buffer[1]);
    g_adc1_buffer[2] = fPulseInterferenceFilter(&g_wi_filter, g_adc1_buffer[2]);

    // 计算校准后的电流值（确保非负）
    float raw_u = (float)(g_adc1_buffer[0] * ADC_VAL_TO_CUR_FACTOR - g_cur_zero_u);
    float raw_v = (float)(g_adc1_buffer[1] * ADC_VAL_TO_CUR_FACTOR - g_cur_zero_v);
    float raw_w = (float)(g_adc1_buffer[2] * ADC_VAL_TO_CUR_FACTOR - g_cur_zero_w);

    g_cur_u = (raw_u > 0) ? raw_u : 0;
    g_cur_v = (raw_v > 0) ? raw_v : 0;
    g_cur_w = (raw_w > 0) ? raw_w : 0;

    *ui = g_cur_u;
    *vi = g_cur_v;
    *wi = g_cur_w;
}

/**
 * @brief 修改ADC采样周期
 * @param compare 比较值
 */
void fAdcSampleChange(u16 compare)
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_4, compare);
}

/**
 * @brief 获取电压值
 * @param voltage 电压值指针
 */
void fAdcGetVoltage(float *voltage)
{
    *voltage = g_adc2_buffer[3] * ADC_VAL_TO_VOL_FACTOR;
}

/**
 * @brief 获取温度值
 * @param ut U mos温度指针
 * @param vt V mos温度指针
 * @param wt W mos温度指针
 * @param temperature 平均温度指针
 */
void fAdcGetTemp(u8 *ut, u8 *vt, u8 *wt, float *temperature)
{
    switch (g_temp_index)
    {
    case 0:
        *ut = g_adc_to_temp[g_adc2_buffer[0]];
        g_temp_index = 1;
        break;
    case 1:
        *vt = g_adc_to_temp[g_adc2_buffer[1]];
        g_temp_index = 2;
        break;
    case 2:
        *wt = g_adc_to_temp[g_adc2_buffer[2]];
        g_temp_index = 3;
        break;
    default:
        *temperature = (float)(*ut + *vt + *wt) / 3.0f;
        g_temp_index = 0;
        break;
    }
}