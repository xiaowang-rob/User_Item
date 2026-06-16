#ifndef __FILTER_H
#define __FILTER_H

#include "bsp.h"
#include "math_fast.h"
/* 限幅滤波法 */
typedef struct
{
    int max_deviation; // 最大允许偏差值
    int last_value;    // 上次滤波值
} tAmplitudeLimitingFilter;

void filter_amplitude_limiting_init(tAmplitudeLimitingFilter *filter, int max_deviation, int initial_value);
int filter_amplitude_limiting(tAmplitudeLimitingFilter *filter, int new_value);

/* 中位值滤波法 */
typedef struct
{
    int *buffer; // 数据缓冲区
    int size;    // 缓冲区大小
    int index;   // 当前索引
} tMedianFilter;

void filter_median_init(tMedianFilter *filter, int *buffer, int size);
int filter_median(tMedianFilter *filter, int new_value);

/* 滑动平均滤波法 */
typedef struct
{
    float32_t *buffer; // 数据缓冲区
    int size;          // 缓冲区大小
    int index;         // 当前索引
    float32_t sum;     // 当前和
    int is_full;       // 缓冲区是否已满
} tMovingAverageFilter;

void filter_moving_avg_init(tMovingAverageFilter *filter, float32_t *buffer, int size);
float32_t fMovingAverageFilter(tMovingAverageFilter *filter, float32_t new_value);

/* 加权滑动平均滤波法 */
typedef struct
{
    int *buffer;      // 数据缓冲区
    int *coefficient; // 加权系数数组
    int size;         // 缓冲区大小
    int index;        // 当前索引
    int coeff_sum;    // 系数和
} tWeightedMovingAverageFilter;

void filter_weighted_moving_avg_init(tWeightedMovingAverageFilter *filter,
                                int *buffer, int *coefficient, int size);
int filter_weighted_moving_avg(tWeightedMovingAverageFilter *filter, int new_value);

/* 一阶滞后滤波法（低通滤波） */
typedef struct
{
    float alpha;      // 滤波系数(0~1)
    float last_value; // 上次滤波值
} tFirstOrderLagFilter;

void filter_first_order_lag_init(tFirstOrderLagFilter *filter, float alpha, float initial_value);
float filter_first_order_lag(tFirstOrderLagFilter *filter, float new_value);

/* 卡尔曼滤波 */
typedef struct
{
    float q; // 过程噪声协方差
    float r; // 测量噪声协方差
    float x; // 估计值
    float p; // 估计误差协方差
    float k; // 卡尔曼增益
} tKalmanFilter;

void filter_kalman_init(tKalmanFilter *filter, float q, float r, float initial_value);
float filter_kalman(tKalmanFilter *filter, float measurement);

/* 防脉冲干扰平均滤波法 */
typedef struct
{
    u16 *buffer; // 数据缓冲区
    u8 size;     // 缓冲区大小
    u8 index;    // 当前索引
} tPulseInterferenceFilter;

void filter_pulse_init(tPulseInterferenceFilter *filter, u16 *buffer, u8 size);
u16 filter_pulse(tPulseInterferenceFilter *filter, u16 new_value);

/* 算术平均滤波法（静态函数，不需要状态） */
int fArithmeticMeanFilter(const int *data_buf, int size);

/* 巴特沃斯低通滤波器*/
typedef struct
{
    arm_biquad_casd_df1_inst_f32 inst;
    float32_t state[4];  // 2 阶 * 2 状态 + block
    float32_t coeffs[5]; // 由 Python 生成填入
} tBW_FilterInstance;

void filter_butterworth_init(tBW_FilterInstance *f, float32_t *coeffs);
float32_t fButterworthFilter_Process(tBW_FilterInstance *f, float32_t input);
void filter_butterworth_reset(tBW_FilterInstance *f);
#endif