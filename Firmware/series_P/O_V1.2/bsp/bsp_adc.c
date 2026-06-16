
#include "bsp_adc.h"
#include "adc.h"
#include "tim.h"
#include "config.h"
#include "string.h"

/* ADC相关配置定义 */

static const float ADC_VAL_TO_CUR_FACTOR = 3.3f * RATE_CURRENT_SAMPLE / 4095.0f;
static const float ADC_VAL_TO_VOL_FACTOR = 3.3f * RATE_VOLTAGE_SAMPLE / 255.0f;
/* ADC采样相关变量 */

static float adc_zero_u = 0;
static float adc_zero_v = 0;
static float adc_zero_w = 0;

static float ADC_VAL_ZERO_U = 4095.0f / 2.0f;
static float ADC_VAL_ZERO_V = 4095.0f / 2.0f;
static float ADC_VAL_ZERO_W = 4095.0f / 2.0f;

static u16 sample_counter = 0;

static u16 adc1_buffer[3];
static u8 adc2_buffer[2];

static bool is_adc_init = false;
static float Vbus = 0;
static float Tempture = 0;

// clang-format off
/* 温度转换查找表 */
static const u8 g_adc_to_temp[256] = {
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
// clang-format on

/**
 * @brief 冒泡排序函数
 * @param arr 待排序的数组
 * @param n 数组大小
 */


/**
 * @brief ADC转换完成回调函数
 * @param hadc ADC句柄指针
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        /* ADC 原始值直接使用，降噪由触发/上溢回调中的 2-shunt 逻辑处理 */
    }
    if (hadc->Instance == ADC2)
    {
        Vbus = adc2_buffer[0] * ADC_VAL_TO_VOL_FACTOR;
        u8 adc_eq = adc2_buffer[1] * 24.0f / (Vbus - 0.3f) + 0.5f;
        Tempture = g_adc_to_temp[adc_eq];
        is_adc_init = true;
    }
}

/**
 * @brief ADC数据采集初始化
 */
void BSP_AdcInit(void)
{
    BSP_AdcSampleChange(TIC_PWM - 1);
    HAL_TIM_Base_Start_IT(&PWM_GET_HTIM);
    HAL_TIM_PWM_Start(&PWM_GET_HTIM, TIM_CHANNEL_4);
    HAL_ADC_Start_DMA(&hadc1, (u32 *)adc1_buffer, 3);
    HAL_ADC_Start_DMA(&hadc2, (u32 *)adc2_buffer, 2);

    while (!is_adc_init)
    {
        BSP_Delay(1);
    }
}

/**
 * @brief 温度从影电压触发采样
 */
void BSP_TempVbusSample(void)
{
    HAL_ADC_Start_DMA(&hadc2, (u32 *)adc2_buffer, 2);
}

// 2-shunt + Clarke：在上溢中断中调用，根据扇区采两相并直接计算 Ialpha/Ibeta
// 省掉一路 ADC + 扇区重构查表
void BSP_SampleCurrent2Shunt(u8 sector, float *ialpha, float *ibeta)
{
    /* 读取 ADC 原始值 */
    float ui = ((float)(adc1_buffer[0] - ADC_VAL_ZERO_U) * ADC_VAL_TO_CUR_FACTOR);
    float vi = ((float)(adc1_buffer[1] - ADC_VAL_ZERO_V) * ADC_VAL_TO_CUR_FACTOR);
    float wi = ((float)(adc1_buffer[2] - ADC_VAL_ZERO_W) * ADC_VAL_TO_CUR_FACTOR);
    float iu, iv, iw;

    /* 根据扇区确定两相：最短导通相由另两相推导 (Ia+Ib+Ic=0) */
    if (sector == 1 || sector == 6) {           /* 最短相=W */
        iv = -vi;  iw = -wi;
        iu = -(iv + iw);
    } else if (sector == 2 || sector == 3) {     /* 最短相=U */
        iu = -ui;  iw = -wi;
        iv = -(iu + iw);
    } else if (sector == 4 || sector == 5) {     /* 最短相=V */
        iu = -ui;  iv = -vi;
        iw = -(iu + iv);
    } else {                                      /* sector 0/7: 零矢量 */
        iu = ui;  iv = vi;  iw = wi;
    }

    /* Clarke 变换 */
    *ialpha = iu;
    *ibeta = (iu + 2.0f * iv) * 0.57735027f;
}

/**
 * @brief 获取电流值
 * @param ui U相电流指针
 * @param vi V相电流指针
 * @param wi W相电流指针
 */
void BSP_AdcGetCurrent(float *ui, float *vi, float *wi)
{
    *ui = ((float)(adc1_buffer[0] - ADC_VAL_ZERO_U) * ADC_VAL_TO_CUR_FACTOR);
    *vi = ((float)(adc1_buffer[1] - ADC_VAL_ZERO_V) * ADC_VAL_TO_CUR_FACTOR);
    *wi = ((float)(adc1_buffer[2] - ADC_VAL_ZERO_W) * ADC_VAL_TO_CUR_FACTOR);
}

/**
 * @brief 修改ADC采样周期
 * @param compare 比较值
 */
void BSP_AdcSampleChange(u16 compare)
{
    __HAL_TIM_SetCompare(&PWM_GET_HTIM, TIM_CHANNEL_4, compare);
}

/**
 * @brief 获取电压值
 * @param voltage 电压值指针
 */
void BSP_AdcGetVoltage(float *voltage)
{
    *voltage = Vbus;
}

void BSP_AdcGetTemp(float *temperature)
{
    *temperature = Tempture;
}

// VESC 式 DC 校准：使能前一阶 LPF 快速收敛
bool BSP_AdcCalibrateCurrent(float *ui_offset, float *vi_offset, float *wi_offset)
{
    const float calib_k = 0.01f;  /* LPF 系数 */
    adc_zero_u += ((float)adc1_buffer[0] - adc_zero_u) * calib_k;
    adc_zero_v += ((float)adc1_buffer[1] - adc_zero_v) * calib_k;
    adc_zero_w += ((float)adc1_buffer[2] - adc_zero_w) * calib_k;
    if (++sample_counter >= 300)
    {
        ADC_VAL_ZERO_U = adc_zero_u;
        ADC_VAL_ZERO_V = adc_zero_v;
        ADC_VAL_ZERO_W = adc_zero_w;
        *ui_offset = ADC_VAL_ZERO_U;
        *vi_offset = ADC_VAL_ZERO_V;
        *wi_offset = ADC_VAL_ZERO_W;
        return true;
    }
    *ui_offset = 0.0f;
    *vi_offset = 0.0f;
    *wi_offset = 0.0f;
    return false;
}

// 空闲时零点漂移跟踪：电机停止时持续慢速 LPF 跟踪
void BSP_AdcIdleTrack(void)
{
    const float idle_k = 0.002f;
    ADC_VAL_ZERO_U += ((float)adc1_buffer[0] - ADC_VAL_ZERO_U) * idle_k;
    ADC_VAL_ZERO_V += ((float)adc1_buffer[1] - ADC_VAL_ZERO_V) * idle_k;
    ADC_VAL_ZERO_W += ((float)adc1_buffer[2] - ADC_VAL_ZERO_W) * idle_k;
}

void BSP_SetAdcCurrentOffset(float ui_offset, float vi_offset, float wi_offset)
{
    ADC_VAL_ZERO_U = ui_offset;
    ADC_VAL_ZERO_V = vi_offset;
    ADC_VAL_ZERO_W = wi_offset;
}