#ifndef __MATH_FAST_H
#define __MATH_FAST_H

#include "main.h"
#include "arm_math.h"
#include "math.h"

/* 数学常量定义 */
#define MATH_PI 3.1415926535f
#define MATH_2PI 6.283185307f
#define MATH_SQRT3 1.732050807f
#define MATH_SQRT3_2 0.8660254035f
#define MATH_INSQRT3 0.5773502693f

/* 函数声明 */
float fRadToRpm(float rad);
float fRpmToRad(float rpm);
float fDegToRad(float deg);
float fRadToDeg(float rad);

void fClarkTransform(float ia, float ib, float ic, float *alpha, float *beta);
void fInvClarkTransform(float alpha, float beta, float *ia, float *ib, float *ic);
void fParkTransform(float alpha, float beta, float angle, float *d, float *q);
void fInvParkTransform(float d, float q, float angle, float *alpha, float *beta);
u32 fFastRoundf(float x);
float fFastAbsf(float x);
u32 HAL_GetTick_us(void);
float fNormalizeAngle02pi(float angle);

#endif /* __MATH_FAST_H */