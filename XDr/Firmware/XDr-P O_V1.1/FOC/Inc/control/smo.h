#ifndef __SMO_H
#define __SMO_H

#include "main.h"
#include "foc_core.h"
/*****************************************无感SMO观测*********************************** */
// 无感 SMO 结构体
typedef struct
{
    u8 pole_pairs; // 极对数
    float Rs;      // 定子电阻
    float Ld;
    float Lq;
    float Psi_f; // 永磁体磁链
    float Ke;    // 反电动势常数
    float J;     // 转动惯量
    float B;     // 摩擦系数

    float dt;  // 控制周期
    float Udc; // 电压
    // 状态变量
    float i_alpha_hat;
    float i_beta_hat;
    float e_alpha;
    float e_beta;
    float e_alpha_filtered;
    float e_beta_filtered;
    float theta;
    float theta_prev;
    float omega;

    bool is_aligned;         // 是否完成初始对齐
    uint32_t alignment_time; // 对齐时间计数
    float startup_gain;      // 启动增益（从0逐渐增加）

    float integrator_alpha; // 电流观测器积分增益
    float integrator_limit; // 积分器限幅

    // 参数
    float k_sl;
    float delta;
    float k_f;
    float max_omega;
} tSMO;
extern tSMO smo;
// 初始化
void fSMO_Init(tMotor motor);
void fSMO_Reset();
// 更新（输入：电压、电流）
void fSMO_MainLoop(float v_alpha, float v_beta,
                   float i_alpha, float i_beta);

// 获取结果
float fSMO_GetTheta();
float fSMO_GetOmega();

#endif