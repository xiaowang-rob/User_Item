#ifndef __STREAM_TRANSMISSION_H
#define __STREAM_TRANSMISSION_H

#include "main.h"

// 反馈参数--流式数据
typedef enum
{
    VBUS = 1,
    TEMP,
    THETA_elec,
    THETA_mech,
    CURRENT_q,
    CURRENT_d,
    CURRENT_alpha,
    CURRENT_beta,
    VOLTAGE_q,
    VOLTAGE_d,
    VOLTAGE_alpha,
    VOLTAGE_beta,
    SPEED_rpm,
    POSITION_motor,
    POSITION_target,
    REF_iq,
    REF_id,
    REF_speed,
    REF_position_motor,
    REF_position_target,
} Data_stream_t;

// 控制静态参数
typedef enum
{
    THETA_OFFSET,
    MOTOR_POLEPAIRS,
    MOTOR_RS,
    MOTOR_LS,
    MOTOR_Psif,
    MOTOR_J,
    MOTOR_B,
    MOTOR_TC,           // 转矩常数
    REDUCTION_RATIO,    // 减速比
    FOC_RUN_MODE,       // 运行模式
    FOC_LOOP_MODE,      // 环路模式
    FOC_STATUS,         // 状态
    f_CURRENT_LOOP,     // 电流环频率
    f_SPEED_LOOP,       // 速度环频率
    f_POSITION_LOOP,    // 位置环频率
    Kp_CURRENT,         // 电流环比例
    Ki_CURRENT,         // 电流环积分
    Kp_SPEED,           // 速度环比例
    Ki_SPEED,           // 速度环积分
    Kp_POSITION,        // 位置环比例
    Ki_POSITION,        // 位置环积分
    Kd_CURRENT,         // 电流环微分
    LIMIT_CURRENT,      // 电流限幅
    LIMIT_SPEED,        // 速度限幅
    LIMIT_POSITION_min, // 位置限幅
    LIMIT_POSITION_max, // 位置限幅
    TOLERANCE_TIME,     // 容忍时间
    TOLERANCE_VOLTAGE,  // 电压容忍度
    TOLERANCE_CURRENT,  // 电流容忍度
    TOLERANCE_SPEED,    // 速度容忍度
    TOLERANCE_POSITION, // 位置容忍度

    STARTUP_POS_GRAD,  // 启动位置斜率
    STARTUP_SPE_GRAD,  // 启动速度斜率
    ALIGN_CURRENT,     // 对齐电流
    ALIGN_TIME,        // 对齐时间
    OPEN_LOOP_CURRENT, // 开环电流
    OPEN_LOOP_SPEED,   // 开环速度
    CHANGE_LOOP_SPEED, // 切环时间
} Parameter_t;

#endif