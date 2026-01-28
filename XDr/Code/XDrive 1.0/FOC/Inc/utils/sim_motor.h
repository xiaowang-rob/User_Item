#ifndef __SIM_MOTOR_H
#define __SIM_MOTOR_H

#include "main.h"

// 电机参数结构体 (直接在定义时初始化)
typedef struct
{
    float Rs;       // 定子电阻 (Ω)
    float Ld;       // d轴电感 (H)
    float Lq;       // q轴电感 (H)
    float psi_f;    // 永磁体磁链 (Wb)
    int pole_pairs; // 极对数
    float J;        // 转动惯量 (kg·m²)
    float B;        // 阻尼系数 (N·m·s)
    float Tl;       // 负载转矩 (N·m)
    float dt;       // 仿真步长 (s)

} MotorParams;

// 电机状态结构体 (包含三相线电流)
typedef struct
{
    float ia; // A相电流 (A)
    float ib; // B相电流 (A)
    float ic; // C相电流 (A) - 注意: ia+ib+ic≈0
    float ialpha, ibeta;
    float iq, id;
    float ualpha, ubeta;
    float uq, ud;
    float omega_m; // 机械角速度 (rad/s)
    float theta;   // 转子电角度 (rad)
    float theta_m; // 转子机械角度 (rad)
    float pos_m;   // 转子位置 (m)
} MotorState;

// 电机仿真对象 (参数直接在定义时初始化)
typedef struct
{
    MotorParams params;
    MotorState state;

} PMSM_Motor;
extern PMSM_Motor gMotor;
void motor_step(float va, float vb, float vc);

#endif // __SIM_MOTOR_H