#include "filter.h"
#include "stddef.h"
#include "string.h"
/**
 * 冒泡排序函数
 * @param arr 待排序的数组
 * @param n 数组大小
 */
void bubble_sort(int arr[], int n)
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
/* 限幅滤波法 */
void amplitude_limiting_init(AmplitudeLimitingFilter *filter, int max_deviation, int initial_value)
{
    filter->max_deviation = max_deviation;
    filter->last_value = initial_value;
}

int amplitude_limiting_filter(AmplitudeLimitingFilter *filter, int new_value)
{
    if ((new_value - filter->last_value > filter->max_deviation) ||
        (filter->last_value - new_value > filter->max_deviation))
    {
        return filter->last_value;
    }
    filter->last_value = new_value;
    return new_value;
}

/* 中位值滤波法 */
void median_filter_init(MedianFilter *filter, int *buffer, int size)
{
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    memset(buffer, 0, size * sizeof(int));
}

int median_filter(MedianFilter *filter, int new_value)
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
    bubble_sort(buf, filter->size);

    // 返回中值
    return buf[(filter->size - 1) / 2];
}

/* 滑动平均滤波法 */
void moving_average_init(MovingAverageFilter *filter, int *buffer, int size)
{
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->sum = 0;
    filter->is_full = 0;
    memset(buffer, 0, size * sizeof(int));
}

int moving_average_filter(MovingAverageFilter *filter, int new_value)
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

/* 加权滑动平均滤波法 */
void weighted_moving_average_init(WeightedMovingAverageFilter *filter,
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

int weighted_moving_average_filter(WeightedMovingAverageFilter *filter, int new_value)
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

/* 一阶滞后滤波法 */
void first_order_lag_init(FirstOrderLagFilter *filter, float alpha, float initial_value)
{
    filter->alpha = alpha;
    filter->last_value = initial_value;
}

float first_order_lag_filter(FirstOrderLagFilter *filter, float new_value)
{
    filter->last_value = filter->alpha * new_value + (1 - filter->alpha) * filter->last_value;
    return filter->last_value;
}

/* 卡尔曼滤波 */
void kalman_init(KalmanFilter *filter, float q, float r, float initial_value)
{
    filter->q = q;
    filter->r = r;
    filter->x = initial_value;
    filter->p = 1.0f;
    filter->k = 0.0f;
}

float kalman_filter(KalmanFilter *filter, float measurement)
{
    // 预测
    filter->p = filter->p + filter->q;

    // 更新
    filter->k = filter->p / (filter->p + filter->r);
    filter->x = filter->x + filter->k * (measurement - filter->x);
    filter->p = (1 - filter->k) * filter->p;

    return filter->x;
}

/* 防脉冲干扰平均滤波法 */
void pulse_interference_init(PulseInterferenceFilter *filter, uint16_t *buffer, uint8_t size)
{
    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    memset(buffer, 0, size * sizeof(uint16_t));
}

uint16_t pulse_interference_filter(PulseInterferenceFilter *filter, uint16_t new_value)
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
    bubble_sort(buf, filter->size);

    // 去掉最大最小值后求平均
    for (i = 1; i < filter->size - 1; i++)
    {
        sum += buf[i];
    }

    return sum / (filter->size - 2);
}

/* 算术平均滤波法 */
int arithmetic_mean_filter(const int *data_buf, int size)
{
    int sum = 0;

    for (int i = 0; i < size; i++)
    {
        sum += data_buf[i];
    }

    return sum / size;
}
