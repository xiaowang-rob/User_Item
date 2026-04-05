#include "adc_dr.h"
#include "adc.h"
#include "tim.h"
#include "drive_parameters.h"
#include "filter.h"

/* ADC相关配置定义 */

static const float ADC_VAL_TO_CUR_FACTOR = 3.3f * rate_CurrentSample / 4095.0f;
static const float ADC_VAL_TO_VOL_FACTOR = 3.3f * rate_VoltageSample / 255.0f;
/* ADC采样相关变量 */
volatile bool g_calibration_flag = true;

static float adc_zero_u = 0;
static float adc_zero_v = 0;
static float adc_zero_w = 0;

static u16 ADC_VAL_ZERO_U = 0;
static u16 ADC_VAL_ZERO_V = 0;
static u16 ADC_VAL_ZERO_W = 0;

static u16 sample_counter = 0;

static u16 adc1_buffer[3];
static u8 adc2_buffer[2];

/* 温度转换查找表 */
static const uint8_t g_adc_to_temp[256] = {
120, 120, 120, 120, 120, 120, 120, 120, 120, 115, 110, 106, 102,  99,  96,  93,
 90,  88,  86,  84,  82,  80,  78,  77,  75,  74,  72,  71,  70,  68,  67,  66,
 65,  64,  63,  62,  61,  60,  59,  58,  57,  57,  56,  55,  54,  54,  53,  52,
 51,  51,  50,  50,  49,  48,  48,  47,  47,  46,  45,  45,  44,  44,  43,  43,
 42,  42,  42,  41,  41,  40,  40,  39,  39,  38,  38,  38,  37,  37,  37,  36,
 36,  35,  35,  35,  34,  34,  34,  33,  33,  33,  32,  32,  32,  31,  31,  31,
 30,  30,  30,  30,  29,  29,  29,  28,  28,  28,  28,  27,  27,  27,  27,  26,
 26,  26,  26,  25,  25,  25,  25,  24,  24,  24,  24,  23,  23,  23,  23,  23,
 22,  22,  22,  22,  21,  21,  21,  21,  21,  20,  20,  20,  20,  20,  19,  19,
 19,  19,  19,  19,  18,  18,  18,  18,  18,  17,  17,  17,  17,  17,  17,  16,
 16,  16,  16,  16,  16,  15,  15,  15,  15,  15,  15,  14,  14,  14,  14,  14,
 14,  14,  13,  13,  13,  13,  13,  13,  12,  12,  12,  12,  12,  12,  12,  11,
 11,  11,  11,  11,  11,  11,  11,  10,  10,  10,  10,  10,  10,  10,   9,   9,
  9,   9,   9,   9,   9,   9,   8,   8,   8,   8,   8,   8,   8,   8,   7,   7,
  7,   7,   7,   7,   7,   7,   7,   6,   6,   6,   6,   6,   6,   6,   6,   6,
  5,   5,   5,   5,   5,   5,   5,   5,   5,   4,   4,   4,   4,   4,   4,   4,};

static u32 sample_time_prev_ms = 0;

// /**
//  * @brief ADC转换完成回调函数
//  * @param hadc ADC句柄指针
//  */
// void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
// {
//     return;
//     if (hadc->Instance == ADC1)
//     {
//     }
// }

/**
 * @brief ADC数据采集初始化
 */
void fAdcDrInit(void)
{
    fAdcSampleChange(2098);
    HAL_TIM_Base_Start_IT(&htim8);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (u32 *)adc1_buffer, 3);
    HAL_ADC_Start_DMA(&hadc2, (u32 *)adc2_buffer, 4);
}

/**
 * @brief ADC2采样触发
 */
void fAdc2Sample(void)
{
    if (HAL_GetTick() - sample_time_prev_ms < TEMP_VBUS_TS_MS)
        return;
    sample_time_prev_ms = HAL_GetTick();
    HAL_ADC_Start_DMA(&hadc2, (u32 *)adc2_buffer, 4);
}

/**
 * @brief 获取电流值
 * @param ui U相电流指针
 * @param vi V相电流指针
 * @param wi W相电流指针
 */
void fAdcGetCurrent(float *ui, float *vi, float *wi)
{
    if (g_calibration_flag)
    {
        adc_zero_u += (float)adc1_buffer[0] * 0.001;
        adc_zero_v += (float)adc1_buffer[1] * 0.001;
        adc_zero_w += (float)adc1_buffer[2] * 0.001;
        if (++sample_counter >= 1000)
        {
            ADC_VAL_ZERO_U = (u16)(adc_zero_u + 0.5f);
            ADC_VAL_ZERO_V = (u16)(adc_zero_v + 0.5f);
            ADC_VAL_ZERO_W = (u16)(adc_zero_w + 0.5f);
            adc_zero_u = 0;
            adc_zero_v = 0;
            adc_zero_w = 0;
            sample_counter = 0;
            g_calibration_flag = false;
        }
    }
    else
    { // 计算校准后的电流值
        *ui = ((float)(adc1_buffer[0] - ADC_VAL_ZERO_U) * ADC_VAL_TO_CUR_FACTOR);
        *vi = ((float)(adc1_buffer[1] - ADC_VAL_ZERO_V) * ADC_VAL_TO_CUR_FACTOR);
        *wi = ((float)(adc1_buffer[2] - ADC_VAL_ZERO_W) * ADC_VAL_TO_CUR_FACTOR);
    }
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
    *voltage = adc2_buffer[0] * ADC_VAL_TO_VOL_FACTOR;
}

/**
 * @brief 获取温度值
 * @param ut U mos温度指针
 * @param vt V mos温度指针
 * @param wt W mos温度指针
 * @param temperature 平均温度指针
 */
void fAdcGetTemp(float *temperature)
{
    u8 adc_eq = adc2_buffer[1] * 24.0f / ADC_VAL_TO_VOL_FACTOR / adc2_buffer[0];
    *temperature = g_adc_to_temp[adc_eq];
}