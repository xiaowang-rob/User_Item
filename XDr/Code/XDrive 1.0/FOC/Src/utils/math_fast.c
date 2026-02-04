#include "math_fast.h"

/**
 * @file math_fast.c
 * @brief 快速数学运算函数实现
 */

/**
 * @brief 将角速度从弧度/秒转换为转/分钟
 * @param rad 输入的角速度（弧度/秒）
 * @return 转/分钟值
 */
float fRadToRpm(float rad)
{
    return rad * 9.549296748f;
}

/**
 * @brief 将转/分钟转换为角速度弧度/秒
 * @param rpm 输入的转/分钟值
 * @return 弧度/秒值
 */
float fRpmToRad(float rpm)
{
    return rpm / 9.549296748f;
}

/**
 * @brief 将角度从度转换为弧度
 * @param deg 输入的角度（度）
 * @return 弧度值
 */
float fDegToRad(float deg)
{
    return deg * 0.017453293f;
}

/**
 * @brief 将角度从弧度转换为度
 * @param rad 输入的弧度值
 * @return 角度值（度）
 */
float fRadToDeg(float rad)
{
    return rad * 57.29577951f;
}

/**
 * @brief Clark 变换 (abc → αβ)(等幅值)
 * @param ia, ib, ic: 三相电流或电压
 * @param alpha, beta: 输出的 αβ 轴分量
 */
void fClarkTransform(float ia, float ib, float ic, float *alpha, float *beta)
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
void fInvClarkTransform(float alpha, float beta, float *ia, float *ib, float *ic)
{
    *ia = alpha;
    *ib = -0.5f * alpha + MATH_SQRT3_2 * beta;
    *ic = -0.5f * alpha - MATH_SQRT3_2 * beta;
}

/**
 * @brief Park 变换 (αβ → dq)
 * @param alpha, beta: αβ 轴分量
 * @param angle: 电角度（弧度）
 * @param d, q: 输出的 dq 轴分量
 */
void fParkTransform(float alpha, float beta, float angle, float *d, float *q)
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
void fInvParkTransform(float d, float q, float angle, float *alpha, float *beta)
{
    float sin_theta, cos_theta;
    sin_theta = arm_sin_f32(angle);
    cos_theta = arm_cos_f32(angle);
    *alpha = d * cos_theta - q * sin_theta;
    *beta = d * sin_theta + q * cos_theta;
}

/**
 * @brief 快速浮点数四舍五入
 * @param x 输入的浮点数
 * @return 四舍五入后的整数
 */
u32 fFastRoundf(float x)
{
    return (u32)(x + 0.5f);
}

/**
 * @brief 快速浮点绝对值计算
 * @param x 输入的浮点数
 * @return 绝对值
 */
float fFastAbsf(float x)
{
    u32 temp = *(u32 *)&x & 0x7FFFFFFF;
    return *(float *)&temp;
}

/**
 * @brief 获取微秒级系统时间戳
 * @return 当前时间（微秒）
 */
u32 HAL_GetTick_us(void)
{
    // 获取当前ms
    u32 m = HAL_GetTick();
    // 获取嘀嗒定时器重装载值
    const u32 tms = SysTick->LOAD + 1;
    // 获取当前滴答定时器计数值
    __IO u32 u = tms - SysTick->VAL;
    // 返回对应的值
    return (m * 1000 + (u * 1000) / tms);
}

/**
 * @brief 将角度标准化到 [0, 2π) 范围
 * @param angle 输入角度（弧度）
 * @return 标准化后的角度 [0, 2π)
 */
float fNormalizeAngle02pi(float angle)
{
    angle = fmodf(angle, MATH_2PI);
    if (angle < 0.0f)
    {
        angle += MATH_2PI;
    }
    return angle;
}