#ifndef __LOOP_CONTROL_H
#define __LOOP_CONTROL_H

#include "main.h"
#include "stdbool.h"
#include "parameter_manager.h"

// 频率分频控制（实现电流/速度/位置环分层更新）
typedef struct
{
    u8 tic;
    u8 current_steps;
    u8 speed_steps;
    u8 current_update_steps; // 修正拼写：updata → update
    u8 speed_update_steps;
    u8 position_update_steps;
    bool current_update;
    bool speed_update;
    bool position_update;
    float Tcur; // 电流环周期
    float Tspd; // 速度环周期
    float Tpos; // 位置环周期
} tFrequencyDivision;

// PI控制器
typedef struct
{
    float kp, ki;
    u8 enable_integral;
    float integral, integral_limit;
    float output_limit, output;
} tPI;

// PID控制器（含微分滤波）
typedef struct
{
    float kp, ki, kd;
    u8 enable_integral;
    float last_error, integral, integral_limit;
    float derivative, last_derivative, derivative_limit, derivative_filter;
    float output_limit, output;
} tPID;

typedef struct
{
    tFrequencyDivision fd;
    tPI PI_iq, PI_id, PI_weakmag, PI_speed;
    tPID PID_pos;
    float max_Vs;
    float position_min, position_max;
} tLoopControl;

extern tLoopControl g_loop_con;

void fFrequencyDivisionUpdate(void);                  // 更新分频计数器和各环更新标志
void fLoopControlInit(Parameter_t param, float Vmax); // 环路参数初始化
void fLoopReset(void);                                // 重置所有控制器状态
float fCurrentLoopUpdate(float ref, float fb);        // q轴电流环
float fMagLoopUpdate(float ref, float fb);            // d轴磁链环
float fWeakMagLoopUpdate(float ud, float uq);         // 弱磁控制
float fSpeedLoopUpdate(float ref, float fb);          // 速度环
float fPositionAbsLoopUpdate(float ref, float fb);    // 绝对位置环
float fPositionRelLoopUpdate(float ref, float fb);    // 相对位置环（带限幅）
void fPVT_SetOmega(float omega);                      // PVT模式下设置位置环速度限幅

#endif