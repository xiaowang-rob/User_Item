#ifndef __MATH_FAST_H
#define __MATH_FAST_H

#include "bsp_interface.h"
#include "arm_math.h"
#include "math.h"

/* 数学常量定义 */
#define MATH_PI 3.1415926535f
#define MATH_2PI 6.2831853f
#define MATH_SQRT3 1.732050807f
#define MATH_SQRT3_2 0.8660254035f
#define MATH_INSQRT3 0.5773502693f
#define MATH_1_SQRT2 0.7071067812f
#define MATH_1_SQRT3 0.5773502691f

/* 函数声明 */
// 一般函数--大型函数 不经常调用

// 内联函数--小函数 经常调用

// 快速限幅
static inline float CLAMP(float val, float min, float max)
{
    return (val < min) ? min : ((val > max) ? max : val);
}
// 快速绝对值
static inline float FABSF(float x)
{
    return __builtin_fabsf(x);
}
// 快速符号函数
static inline float FSIGN(float x)
{
    return (x > 0.0f) - (x < 0.0f); // 分支消除
}
// 快速浮点数四舍五入
static inline u32 fFastRoundf(float x)
{
    return (u32)(x + 0.5f);
}

// 将角度标准化到 [0, 360) 范围
static inline float fNormalizeAngle_0_360(float angle)
{
    angle = fmodf(angle, 360);
    if (angle < 0.0f)
    {
        angle += 360;
    }
    return angle;
}
// 将角度标准化到[-π, π]范围
static inline float fNormalizeAngle_180(float angle)
{
    /* 利用 fmodf 将角度映射到 [-2π, 2π]，再调整到 [-π, π] */
    angle = fmodf(angle + 180, 360);
    if (angle < 0.0f)
        angle += 360;
    return angle - 180;
}

/**
 * @brief Clark 变换 (abc → αβ)(等幅值)
 * @param ia, ib, ic: 三相电流或电压
 * @param alpha, beta: 输出的 αβ 轴分量
 */
static inline void fClarkTransform(float ia, float ib, float ic, float *alpha, float *beta)
{
    // 使用幅值不变变换（系数 2/3）
    // 简化电流 ia+ib+ic=0
    *alpha = ia;
    *beta = MATH_INSQRT3 * (ib - ic);
}
/**
 * @brief Clark 反变换 (αβ → abc)(等幅值)
 * @param alpha, beta: αβ 轴分量
 * @param ia, ib, ic: 输出的三相值
 */
static inline void fInvClarkTransform(float alpha, float beta, float *ia, float *ib, float *ic)
{
    *ia = alpha;
    *ib = -0.5f * alpha + MATH_SQRT3_2 * beta;
    *ic = -0.5f * alpha - MATH_SQRT3_2 * beta;
}
/**
 * @brief Park 变换 (αβ → dq)
 * @param alpha, beta: αβ 轴分量
 * @param angle: 电角度（角度）
 * @param d, q: 输出的 dq 轴分量
 */
static inline void fParkTransform(float alpha, float beta, float angle, float *d, float *q)
{
    float sin_theta, cos_theta;
    arm_sin_cos_f32(angle, &sin_theta, &cos_theta);

    *d = alpha * cos_theta + beta * sin_theta;
    *q = -alpha * sin_theta + beta * cos_theta;
}
/**
 * @brief Park 反变换 (dq → αβ)
 * @param d, q: dq 轴分量
 * @param angle: 电角度（角度）
 * @param alpha, beta: 输出的 αβ 轴分量
 */
static inline void fInvParkTransform(float d, float q, float angle, float *alpha, float *beta)
{
    float sin_theta, cos_theta;
    arm_sin_cos_f32(angle, &sin_theta, &cos_theta);
    *alpha = d * cos_theta - q * sin_theta;
    *beta = d * sin_theta + q * cos_theta;
}

#endif /* __MATH_FAST_H */