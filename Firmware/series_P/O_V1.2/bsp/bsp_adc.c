
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
static void vBubbleSort(int arr[], int n)
{
    if (arr == NULL || n <= 1)
    {
        return; // 如果数组为空或只有一个元素，直接返回
    }

    int i, j;
    int temp;
    int swapped; // 优化标志，如果某一轮没有交换，说明已经有序

    for (i = 0; i < n - 1; i++)
    {
        swapped = 0; // 每轮开始前重置交换标志

        // 每轮将最大的元素"冒泡"到末尾
        for (j = 0; j < n - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                // 交换相邻元素
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
                swapped = 1; // 标记发生了交换
            }
        }

        // 如果这一轮没有发生交换，说明数组已经有序，可以提前结束
        if (!swapped)
        {
            break;
        }
    }
}
typedef struct
{
    int *buffer; // 数据缓冲区
    int size;    // 缓冲区大小
    int index;   // 当前索引
} tMedianFilter;
/**
 * @brief 中位值滤波法初始化
 * @param filter 滤波器结构体指针
 * @param buffer 数据缓冲区指针
 * @param size 缓冲区大小
 */
static void fMedianFilterInit(tMedianFilter *filter, int *buffer, int size)
{
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    memset(buffer, 0, size * sizeof(int));
}

/**
 * @brief 中位值滤波法处理
 * @param filter 滤波器结构体指针
 * @param new_value 新的采样值
 * @return 滤波后的值
 */
static int fMedianFilter(tMedianFilter *filter, int new_value)
{
    int i;
    int buf[filter->size];

    // 更新缓冲区
    filter->buffer[filter->index] = new_value;
    filter->index = (filter->index + 1) % filter->size;

    // 复制数据到临时数组
    for (i = 0; i < filter->size; i++)
    {
        buf[i] = filter->buffer[i];
    }

    // 排序
    vBubbleSort(buf, filter->size);

    // 返回中值
    return buf[(filter->size - 1) / 2];
}

tMedianFilter Ia_Filter;
tMedianFilter Ib_Filter;
tMedianFilter Ic_Filter;

int Ia_buf[MED_FILTER_SIZE];
int Ib_buf[MED_FILTER_SIZE];
int Ic_buf[MED_FILTER_SIZE];
/**
 * @brief ADC转换完成回调函数
 * @param hadc ADC句柄指针
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc1_buffer[0] = fMedianFilter(&Ia_Filter, adc1_buffer[0]);
        adc1_buffer[1] = fMedianFilter(&Ib_Filter, adc1_buffer[1]);
        adc1_buffer[2] = fMedianFilter(&Ic_Filter, adc1_buffer[2]);
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
    fMedianFilterInit(&Ia_Filter, Ia_buf, MED_FILTER_SIZE);
    fMedianFilterInit(&Ib_Filter, Ib_buf, MED_FILTER_SIZE);
    fMedianFilterInit(&Ic_Filter, Ic_buf, MED_FILTER_SIZE);
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

// 校准电流偏移
bool BSP_AdcCalibrateCurrent(float *ui_offset, float *vi_offset, float *wi_offset)
{
    adc_zero_u += (float)adc1_buffer[0] * 0.001;
    adc_zero_v += (float)adc1_buffer[1] * 0.001;
    adc_zero_w += (float)adc1_buffer[2] * 0.001;
    if (++sample_counter >= 1000)
    {
        ADC_VAL_ZERO_U = adc_zero_u;
        ADC_VAL_ZERO_V = adc_zero_v;
        ADC_VAL_ZERO_W = adc_zero_w;
        adc_zero_u = 0;
        adc_zero_v = 0;
        adc_zero_w = 0;
        sample_counter = 0;
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

void BSP_SetAdcCurrentOffset(float ui_offset, float vi_offset, float wi_offset)
{
    ADC_VAL_ZERO_U = ui_offset;
    ADC_VAL_ZERO_V = vi_offset;
    ADC_VAL_ZERO_W = wi_offset;
}