#ifndef __LOOP_CONTROL_H
#define __LOOP_CONTROL_H

#include "bsp.h"
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
    float dt;
    float integral, integral_limit;
    float output_limit, output;
} tPI;

// PID控制器（含微分滤波）
typedef struct
{
    float dt;               // 时间间隔（秒）
    float kp;               // 比例系数（离散域）
    float ki;               // 积分系数（离散域 = Ki_cont * Ts）
    float kd;               // 微分系数（离散域 = Kd_cont / Ts）
    float integral;         // 积分累加项
    float last_error;       // 上一次误差
    float derivative;       // 滤波后的微分值
    float output;           // 当前输出（速度指令）
    float output_limit;     // 输出限幅（最大速度 ±rad/s）
    float integral_limit;   // 积分项限幅
    float derivative_limit; // 微分项限幅
    float alpha;            // 微分滤波系数 (0.1~0.3)
} tPID;

typedef struct
{
    tFrequencyDivision fd;
    tPI PI_iq, PI_id, PI_weakmag, PI_speed;
    tPID PID_pos;
    float max_Vs;
    float position_min, position_max;
} tLoopControl;

extern tLoopControl loop_con;

void fFrequencyDivisionUpdate(void);                // 更新分频计数器和各环更新标志
void fLoopControlInit(tParameter param, float Udc); // 环路参数初始化
void fLoopReset(void);                              // 重置所有控制器状态
float fCurrentLoopUpdate(float ref, float fb);      // q轴电流环
float fMagLoopUpdate(float ref, float fb);          // d轴磁链环
float fWeakMagLoopUpdate(float ud, float uq);       // 弱磁控制
float fSpeedLoopUpdate(float ref, float fb);        // 速度环
float fPositionRelLoopUpdate(float ref, float fb);  // 相对位置环（带限幅）

#endif