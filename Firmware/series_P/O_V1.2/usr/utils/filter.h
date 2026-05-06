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

void fAmplitudeLimitingInit(tAmplitudeLimitingFilter *filter, int max_deviation, int initial_value);
int fAmplitudeLimitingFilter(tAmplitudeLimitingFilter *filter, int new_value);

/* 中位值滤波法 */
typedef struct
{
    int *buffer; // 数据缓冲区
    int size;    // 缓冲区大小
    int index;   // 当前索引
} tMedianFilter;

void fMedianFilterInit(tMedianFilter *filter, int *buffer, int size);
int fMedianFilter(tMedianFilter *filter, int new_value);

/* 滑动平均滤波法 */
typedef struct
{
    float32_t *buffer; // 数据缓冲区
    int size;          // 缓冲区大小
    int index;         // 当前索引
    float32_t sum;     // 当前和
    int is_full;       // 缓冲区是否已满
} tMovingAverageFilter;

void fMovingAverageInit(tMovingAverageFilter *filter, float32_t *buffer, int size);
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

void fWeightedMovingAverageInit(tWeightedMovingAverageFilter *filter,
                                int *buffer, int *coefficient, int size);
int fWeightedMovingAverageFilter(tWeightedMovingAverageFilter *filter, int new_value);

/* 一阶滞后滤波法（低通滤波） */
typedef struct
{
    float alpha;      // 滤波系数(0~1)
    float last_value; // 上次滤波值
} tFirstOrderLagFilter;

void fFirstOrderLagInit(tFirstOrderLagFilter *filter, float alpha, float initial_value);
float fFirstOrderLagFilter(tFirstOrderLagFilter *filter, float new_value);

/* 卡尔曼滤波 */
typedef struct
{
    float q; // 过程噪声协方差
    float r; // 测量噪声协方差
    float x; // 估计值
    float p; // 估计误差协方差
    float k; // 卡尔曼增益
} tKalmanFilter;

void fKalmanInit(tKalmanFilter *filter, float q, float r, float initial_value);
float fKalmanFilter(tKalmanFilter *filter, float measurement);

/* 防脉冲干扰平均滤波法 */
typedef struct
{
    u16 *buffer; // 数据缓冲区
    u8 size;     // 缓冲区大小
    u8 index;    // 当前索引
} tPulseInterferenceFilter;

void fPulseInterferenceInit(tPulseInterferenceFilter *filter, u16 *buffer, u8 size);
u16 fPulseInterferenceFilter(tPulseInterferenceFilter *filter, u16 new_value);

/* 算术平均滤波法（静态函数，不需要状态） */
int fArithmeticMeanFilter(const int *data_buf, int size);

/* 巴特沃斯低通滤波器*/
typedef struct
{
    arm_biquad_casd_df1_inst_f32 inst;
    float32_t state[4];  // 2 阶 * 2 状态 + block
    float32_t coeffs[5]; // 由 Python 生成填入
} tBW_FilterInstance;

void fButterworthFilter_Init(tBW_FilterInstance *f, float32_t *coeffs);
float32_t fButterworthFilter_Process(tBW_FilterInstance *f, float32_t input);
void fButterworthFilter_Reset(tBW_FilterInstance *f);
#endif