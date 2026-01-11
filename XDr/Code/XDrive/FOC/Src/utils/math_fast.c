#include "math_fast.h"

float rad_to_rpm(float rad)
{
    return rad * 9.549296748f;
}
float rpm_to_rad(float rpm)
{
    return rpm / 9.549296748f;
}
float deg_to_rad(float deg)
{
    return deg * 0.017453293f;
}
float rad_to_deg(float rad)
{
    return rad * 57.29577951f;
}
/**
 * @brief Clark 变换 (abc → αβ)
 * @param ia, ib, ic: 三相电流或电压
 * @param alpha, beta: 输出的 αβ 轴分量
 */
void clark_transform(float ia, float ib, float ic, float *alpha, float *beta)
{
    // 使用功率不变变换（系数 2/3）
    *alpha = ia;
    *beta = insqrt3 * (2 * ib - ic);

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
    *ib = -0.5f * alpha + sqrt3_2 * beta;
    *ic = -0.5f * alpha - sqrt3_2 * beta;
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
    sin_theta = arm_sin_f32(angle);
    cos_theta = arm_cos_f32(angle);

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
    sin_theta = arm_sin_f32(angle);
    cos_theta = arm_cos_f32(angle);
    *alpha = d * cos_theta - q * sin_theta;
    *beta = d * sin_theta + q * cos_theta;
}

u32 fast_roundf(float x)
{
    return (u32)(x + 0.5f);
}
float fast_absf(float x)
{
    u32 temp = *(u32 *)&x & 0x7FFFFFFF;
    return *(float *)&temp;
}
u32 HAL_GetTick_us()
{
    // 获取当前ms
    u32 m = HAL_GetTick();
    // 获取嘀嗒定时器重装载值
    const u32 tms = SysTick->LOAD + 1;
    // 获取当前滴答定时器计数值
    __IO u32 u = tms - SysTick->VAL;
    // 返还对应的值
    return (m * 1000 + (u * 1000) / tms);
}

float normalize_angle_0_2pi(float angle)
{
    angle = fmod(angle, M2_PI);
    if (angle < 0.0f)
    {
        angle += M2_PI;
    }
    return angle;
}
