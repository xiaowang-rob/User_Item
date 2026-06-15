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
    float t_cur; // 电流环周期
    float t_spd; // 速度环周期
    float t_pos; // 位置环周期
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


// TODO(xdr): 命名规范说明
// 以下函数中 PI_init/PI_update/PID_init 使用全大写前缀，
// 在C语言中全大写通常留给 #define 宏。
// VESC风格建议改为小写下划线 + 模块前缀:
//   PI_init()           → loop_pi_init()
//   PI_update()         → loop_pi_update()
//   PID_init()          → loop_pid_init()
// 函数不应使用全大写命名。
void fFrequencyDivisionUpdate(void);
void fLoopControlInit(tParameter *param, float Udc);
void fLoopReset(float Udc);
float fCurrentLoopUpdate(float ref, float fb);
float fMagLoopUpdate(float ref, float fb);
float fWeakMagLoopUpdate(float ud, float uq);
float fSpeedLoopUpdate(float ref, float fb);
float fPositionRelLoopUpdate(float ref, float fb);


#endif
extern tLoopControl g_loop_con;
