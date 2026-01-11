#ifndef __MATH_FASH_H
#define __MATH_FASH_H

#include "main.h"
#include "arm_math.h"
#include "math.h"

#define M_PI 3.1415926535f
#define M2_PI M_PI * 2
#define sqrt3 1.732050807f
#define sqrt3_2 sqrt3 / 2
#define insqrt3 1 / sqrt3

float rad_to_rpm(float rad);
float rpm_to_rad(float rpm);
float deg_to_rad(float deg);
float rad_to_deg(float rad);

void clark_transform(float ia, float ib, float ic, float *alpha, float *beta);
void inv_clark_transform(float alpha, float beta, float *ia, float *ib, float *ic);
void park_transform(float alpha, float beta, float angle, float *d, float *q);
void inv_park_transform(float d, float q, float angle, float *alpha, float *beta);
u32 fast_roundf(float x);
float fast_absf(float x);
u32 HAL_GetTick_us();
float normalize_angle_0_2pi(float angle);
#endif