#include "filter.h"
#include "stddef.h"
#include "string.h"

/**
 * @file filter.c
 * @brief 各种数字滤波算法实现
 */

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

/**
 * @brief 限幅滤波法初始化
 * @param filter 滤波器结构体指针
 * @param max_deviation 最大允许偏差值
 * @param initial_value 初始值
 */
void fAmplitudeLimitingInit(tAmplitudeLimitingFilter *filter, int max_deviation, int initial_value)
{
    filter->max_deviation = max_deviation;
    filter->last_value = initial_value;
}

/**
 * @brief 限幅滤波法处理
 * @param filter 滤波器结构体指针
 * @param new_value 新的采样值
 * @return 滤波后的值
 */
int fAmplitudeLimitingFilter(tAmplitudeLimitingFilter *filter, int new_value)
{
    if ((new_value - filter->last_value > filter->max_deviation) ||
        (filter->last_value - new_value > filter->max_deviation))
    {
        return filter->last_value;
    }
    filter->last_value = new_value;
    return new_value;
}

/**
 * @brief 中位值滤波法初始化
 * @param filter 滤波器结构体指针
 * @param buffer 数据缓冲区指针
 * @param size 缓冲区大小
 */
void fMedianFilterInit(tMedianFilter *filter, int *buffer, int size)
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
int fMedianFilter(tMedianFilter *filter, int new_value)
{
    int i, j, temp;
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

/**
 * @brief 滑动平均滤波法初始化
 * @param filter 滤波器结构体指针
 * @param buffer 数据缓冲区指针
 * @param size 缓冲区大小
 */
void fMovingAverageInit(tMovingAverageFilter *filter, int *buffer, int size)
{
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->sum = 0;
    filter->is_full = 0;
    memset(buffer, 0, size * sizeof(int));
}

/**
 * @brief 滑动平均滤波法处理
 * @param filter 滤波器结构体指针
 * @param new_value 新的采样值
 * @return 滤波后的值
 */
int fMovingAverageFilter(tMovingAverageFilter *filter, int new_value)
{
    // 减去即将被替换的值（如果缓冲区已满）
    if (filter->is_full)
    {
        filter->sum -= filter->buffer[filter->index];
    }

    // 更新缓冲区
    filter->buffer[filter->index] = new_value;
    filter->sum += new_value;

    // 更新索引
    filter->index = (filter->index + 1) % filter->size;

    // 检查缓冲区是否已满
    if (filter->index == 0)
    {
        filter->is_full = 1;
    }

    // 计算平均值
    if (filter->is_full)
    {
        return filter->sum / filter->size;
    }
    else
    {
        return filter->sum / (filter->index + 1);
    }
}

/**
 * @brief 加权滑动平均滤波法初始化
 * @param filter 滤波器结构体指针
 * @param buffer 数据缓冲区指针
 * @param coefficient 加权系数数组指针
 * @param size 缓冲区大小
 */
void fWeightedMovingAverageInit(tWeightedMovingAverageFilter *filter,
                                int *buffer, int *coefficient, int size)
{
    filter->buffer = buffer;
    filter->coefficient = coefficient;
    filter->size = size;
    filter->index = 0;
    filter->coeff_sum = 0;

    // 计算系数和
    for (int i = 0; i < size; i++)
    {
        filter->coeff_sum += coefficient[i];
    }

    memset(buffer, 0, size * sizeof(int));
}

/**
 * @brief 加权滑动平均滤波法处理
 * @param filter 滤波器结构体指针
 * @param new_value 新的采样值
 * @return 滤波后的值
 */
int fWeightedMovingAverageFilter(tWeightedMovingAverageFilter *filter, int new_value)
{
    int sum = 0;

    // 更新缓冲区
    filter->buffer[filter->index] = new_value;

    // 计算加权和
    for (int i = 0; i < filter->size; i++)
    {
        int buf_index = (filter->index + i) % filter->size;
        sum += filter->buffer[buf_index] * filter->coefficient[i];
    }

    // 更新索引
    filter->index = (filter->index + 1) % filter->size;

    return sum / filter->coeff_sum;
}

/**
 * @brief 一阶滞后滤波法初始化
 * @param filter 滤波器结构体指针
 * @param alpha 滤波系数(0~1)
 * @param initial_value 初始值
 */
void fFirstOrderLagInit(tFirstOrderLagFilter *filter, float alpha, float initial_value)
{
    filter->alpha = alpha;
    filter->last_value = initial_value;
}

/**
 * @brief 一阶滞后滤波法处理
 * @param filter 滤波器结构体指针
 * @param new_value 新的采样值
 * @return 滤波后的值
 */
float fFirstOrderLagFilter(tFirstOrderLagFilter *filter, float new_value)
{
    filter->last_value = filter->alpha * new_value + (1 - filter->alpha) * filter->last_value;
    return filter->last_value;
}

/**
 * @brief 卡尔曼滤波初始化
 * @param filter 滤波器结构体指针
 * @param q 过程噪声协方差
 * @param r 测量噪声协方差
 * @param initial_value 初始值
 */
void fKalmanInit(tKalmanFilter *filter, float q, float r, float initial_value)
{
    filter->q = q;
    filter->r = r;
    filter->x = initial_value;
    filter->p = 1.0f;
    filter->k = 0.0f;
}

/**
 * @brief 卡尔曼滤波处理
 * @param filter 滤波器结构体指针
 * @param measurement 测量值
 * @return 滤波后的值
 */
float fKalmanFilter(tKalmanFilter *filter, float measurement)
{
    // 预测
    filter->p = filter->p + filter->q;

    // 更新
    filter->k = filter->p / (filter->p + filter->r);
    filter->x = filter->x + filter->k * (measurement - filter->x);
    filter->p = (1 - filter->k) * filter->p;

    return filter->x;
}

/**
 * @brief 防脉冲干扰平均滤波法初始化
 * @param filter 滤波器结构体指针
 * @param buffer 数据缓冲区指针
 * @param size 缓冲区大小
 */
void fPulseInterferenceInit(tPulseInterferenceFilter *filter, u16 *buffer, u8 size)
{
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    memset(buffer, 0, size * sizeof(u16));
}

/**
 * @brief 防脉冲干扰平均滤波法处理
 * @param filter 滤波器结构体指针
 * @param new_value 新的采样值
 * @return 滤波后的值
 */
u16 fPulseInterferenceFilter(tPulseInterferenceFilter *filter, u16 new_value)
{
    int i, sum = 0;
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

    // 去掉最大最小值后求平均
    for (i = 1; i < filter->size - 1; i++)
    {
        sum += buf[i];
    }

    return sum / (filter->size - 2);
}

/**
 * @brief 算术平均滤波法处理
 * @param data_buf 数据数组指针
 * @param size 数组大小
 * @return 平均值
 */
int fArithmeticMeanFilter(const int *data_buf, int size)
{
    int sum = 0;

    for (int i = 0; i < size; i++)
    {
        sum += data_buf[i];
    }

    return sum / size;
}
/**
 * @brief 巴特沃斯滤波器初始化
 * @param[in] f       滤波器实例指针，指向tBW_FilterInstance结构体
 * @param[in] coeffs  滤波器系数数组指针，格式为{b0, b1, b2, a1, a2}
 * @retval 无
 * @note 系数需预先通过双线性变换法计算（推荐使用Python scipy.signal.butter生成）
 */
void fButterworthFilter_Init(tBW_FilterInstance *f, float32_t *coeffs)
{
    /* 拷贝系数到实例内部缓冲区，避免外部数组被意外修改 */
    for (int i = 0; i < 5; i++)
    {
        f->coeffs[i] = coeffs[i];
    }

    /* 初始化CMSIS-DSP滤波器结构体
     * 参数说明：
     *   &f->inst  : 滤波器实例
     *   1         : 二阶节数量（2阶巴特沃斯对应1个二阶节）
     *   f->coeffs : 系数数组
     *   f->state  : 状态变量缓冲区（长度=2*numStages=4）
     */
    arm_biquad_cascade_df1_init_f32(&f->inst, 1, f->coeffs, f->state);
}

/**
 * @brief 巴特沃斯滤波器单步处理
 * @param[in] f     滤波器实例指针
 * @param[in] input 当前采样输入值
 * @retval float32_t 滤波后的输出值
 * @note 该函数为实时调用，执行时间约几十 cycles（FPU使能）
 */
float32_t fButterworthFilter_Process(tBW_FilterInstance *f, float32_t input)
{
    float32_t output;

    /* 调用CMSIS-DSP优化函数执行滤波
     * 输入输出指针传递，长度为1（单通道单采样）
     */
    arm_biquad_cascade_df1_f32(&f->inst, &input, &output, 1);

    return output;
}

/**
 * @brief 滤波器状态清零（用于启动或重置）
 * @param[in] f 滤波器实例指针
 * @retval 无
 * @note 避免重启时状态变量残留导致输出跳变
 */
void fButterworthFilter_Reset(tBW_FilterInstance *f)
{
    /* 清空状态缓冲区，共4个float32_t（2阶*2状态） */
    memset(f->state, 0, sizeof(float32_t) * 4);
}