#ifndef __MATH_FASH_H
#define __MATH_FASH_H

#include "main.h"

#define M_PI 3.1415926535f
#define M2_PI M_PI * 2
#define sqrt3 1.732050807f
#define sqrt3_2 aqrt3 / 2

// 定义查找表大小
#define SINCOS_TABLE_SIZE 1024
#define SINCOS_TABLE_MASK (SINCOS_TABLE_SIZE - 1)

void clark_transform(float ia, float ib, float ic, float *alpha, float *beta);
void inv_clark_transform(float alpha, float beta, float *ia, float *ib, float *ic);
void park_transform(float alpha, float beta, float angle, float *d, float *q);
void inv_park_transform(float d, float q, float angle, float *alpha, float *beta);

/**
 * @brief 初始化快速三角函数表
 */
void init_fast_sincos_table(void);
float fast_sin(float angle_rad);
float fast_cos(float angle_rad);

#endif