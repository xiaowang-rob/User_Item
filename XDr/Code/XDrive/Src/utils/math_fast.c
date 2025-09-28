#include "math_fast.h"
#include "math.h"
/**
 * @brief Clark 变换 (abc → αβ)
 * @param ia, ib, ic: 三相电流或电压
 * @param alpha, beta: 输出的 αβ 轴分量
 */
void clark_transform(float ia, float ib, float ic, float *alpha, float *beta)
{
    // 使用功率不变变换（系数 2/3）
    *alpha = ia;
    *beta = (1.0f / sqrtf(3.0f)) * (ib - ic);
    // 或者更精确的变换：
    // *alpha = ia;
    // *beta = (1.0f / sqrtf(3.0f)) * (2 * ib + ia);
}
/**
 * @brief Clark 反变换 (αβ → abc)
 * @param alpha, beta: αβ 轴分量
 * @param ia, ib, ic: 输出的三相值
 */
void inv_clark_transform(float alpha, float beta, float *ia, float *ib, float *ic)
{
    *ia = alpha;
    *ib = -0.5f * alpha + (sqrtf(3.0f) / 2.0f) * beta;
    *ic = -0.5f * alpha - (sqrtf(3.0f) / 2.0f) * beta;
}
/**
 * @brief Park 变换 (αβ → dq)
 * @param alpha, beta: αβ 轴分量
 * @param angle: 电角度（弧度）
 * @param d, q: 输出的 dq 轴分量
 */
void park_transform(float alpha, float beta, float angle, float *d, float *q)
{
    float sin_theta, cos_theta;
    sin_theta = fast_sin(angle);
    cos_theta = fast_cos(angle);

    *d = alpha * cos_theta + beta * sin_theta;
    *q = -alpha * sin_theta + beta * cos_theta;
}
/**
 * @brief Park 反变换 (dq → αβ)
 * @param d, q: dq 轴分量
 * @param angle: 电角度（弧度）
 * @param alpha, beta: 输出的 αβ 轴分量
 */
void inv_park_transform(float d, float q, float angle, float *alpha, float *beta)
{
    float sin_theta, cos_theta;
    sin_theta = fast_sin(angle);
    cos_theta = fast_cos(angle);
    *alpha = d * cos_theta - q * sin_theta;
    *beta = d * sin_theta + q * cos_theta;
}

// 预计算的 sin/cos 表（在初始化时填充）
static float fast_sin_table[SINCOS_TABLE_SIZE];
static float fast_cos_table[SINCOS_TABLE_SIZE];

// 初始化查表（仅在系统启动时调用一次）
void init_fast_sincos_table(void)
{
    for (int i = 0; i < SINCOS_TABLE_SIZE; i++)
    {
        float angle = (M2_PI * i) / SINCOS_TABLE_SIZE;
        fast_sin_table[i] = sinf(angle);
        fast_cos_table[i] = cosf(angle);
    }
}

/**
 * @brief 快速 sin 函数（查表 + 线性插值）
 * @param angle_rad: 输入角度（弧度）
 * @return sin(angle_rad)
 */
float fast_sin(float angle_rad)
{
    // 角度归一化到 [0, 2π]
    angle_rad = fmodf(angle_rad, M2_PI);
    if (angle_rad < 0)
        angle_rad += M2_PI;

    // 映射到表索引
    float index_f = (angle_rad * SINCOS_TABLE_SIZE) / (M2_PI);
    u16 index = (u16)index_f;
    float frac = index_f - index;

    // 获取相邻两个值
    u16 idx0 = index & SINCOS_TABLE_MASK;
    u16 idx1 = (index + 1) & SINCOS_TABLE_MASK;

    // 线性插值
    return fast_sin_table[idx0] + frac * (fast_sin_table[idx1] - fast_sin_table[idx0]);
}

/**
 * @brief 快速 cos 函数（查表 + 线性插值）
 * @param angle_rad: 输入角度（弧度）
 * @return cos(angle_rad)
 */
float fast_cos(float angle_rad)
{
    // 角度归一化到 [0, 2π]
    angle_rad = fmodf(angle_rad, M2_PI);
    if (angle_rad < 0)
        angle_rad += M2_PI;

    // 映射到表索引
    float index_f = (angle_rad * SINCOS_TABLE_SIZE) / (M2_PI);
    u16 index = (u16)index_f;
    float frac = index_f - index;

    // 获取相邻两个值
    u16 idx0 = index & SINCOS_TABLE_MASK;
    u16 idx1 = (index + 1) & SINCOS_TABLE_MASK;

    // 线性插值
    return fast_cos_table[idx0] + frac * (fast_cos_table[idx1] - fast_cos_table[idx0]);
}