#ifndef __AUTO_CALIBRATION_H
#define __AUTO_CALIBRATION_H

#include "main.h"

typedef struct
{
    float Vbus;            // 母线电压
    float Reduction_ratio; // 减速比
    float Rs;              // 定子电阻
    float Ls;              // 定子电感
    float Psi_f;           // 永磁体磁链
    float pole_pairs;      // 极对数
    float J;               // 转动惯量
    float B;               // 摩擦系数
    float torque_constant; // 转矩常数

} Motor_t;
extern Motor_t g_Motor;
#endif