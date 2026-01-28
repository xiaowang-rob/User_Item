#ifndef __FILTER_H
#define __FILTER_H


#include "main.h"
/* 限幅滤波法 */
typedef struct
{
    int max_deviation; // 最大允许偏差值
    int last_value;    // 上次滤波值
} AmplitudeLimitingFilter;

void amplitude_limiting_init(AmplitudeLimitingFilter *filter, int max_deviation, int initial_value);
int amplitude_limiting_filter(AmplitudeLimitingFilter *filter, int new_value);

/* 中位值滤波法 */
typedef struct
{
    int *buffer; // 数据缓冲区
    int size;    // 缓冲区大小
    int index;   // 当前索引
} MedianFilter;

void median_filter_init(MedianFilter *filter, int *buffer, int size);
int median_filter(MedianFilter *filter, int new_value);

/* 滑动平均滤波法 */
typedef struct
{
    int *buffer; // 数据缓冲区
    int size;    // 缓冲区大小
    int index;   // 当前索引
    int sum;     // 当前和
    int is_full; // 缓冲区是否已满
} MovingAverageFilter;

void moving_average_init(MovingAverageFilter *filter, int *buffer, int size);
int moving_average_filter(MovingAverageFilter *filter, int new_value);

/* 加权滑动平均滤波法 */
typedef struct
{
    int *buffer;      // 数据缓冲区
    int *coefficient; // 加权系数数组
    int size;         // 缓冲区大小
    int index;        // 当前索引
    int coeff_sum;    // 系数和
} WeightedMovingAverageFilter;

void weighted_moving_average_init(WeightedMovingAverageFilter *filter,
                                  int *buffer, int *coefficient, int size);
int weighted_moving_average_filter(WeightedMovingAverageFilter *filter, int new_value);

/* 一阶滞后滤波法（低通滤波） */
typedef struct
{
    float alpha;      // 滤波系数(0~1)
    float last_value; // 上次滤波值
} FirstOrderLagFilter;

void first_order_lag_init(FirstOrderLagFilter *filter, float alpha, float initial_value);
float first_order_lag_filter(FirstOrderLagFilter *filter, float new_value);

/* 卡尔曼滤波 */
typedef struct
{
    float q; // 过程噪声协方差
    float r; // 测量噪声协方差
    float x; // 估计值
    float p; // 估计误差协方差
    float k; // 卡尔曼增益
} KalmanFilter;

void kalman_init(KalmanFilter *filter, float q, float r, float initial_value);
float kalman_filter(KalmanFilter *filter, float measurement);

/* 防脉冲干扰平均滤波法 */
typedef struct
{
    uint16_t *buffer; // 数据缓冲区
    uint8_t size;     // 缓冲区大小
    uint8_t index;    // 当前索引
} PulseInterferenceFilter;

void pulse_interference_init(PulseInterferenceFilter *filter, uint16_t *buffer, uint8_t size);
uint16_t pulse_interference_filter(PulseInterferenceFilter *filter, uint16_t new_value);

/* 算术平均滤波法（静态函数，不需要状态） */
int arithmetic_mean_filter(const int *data_buf, int size);

#endif