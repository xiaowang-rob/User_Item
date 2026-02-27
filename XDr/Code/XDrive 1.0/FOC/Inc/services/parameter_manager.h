#ifndef __PARAMETER_MANAGER_H__
#define __PARAMETER_MANAGER_H__

#include "main.h"

#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000

typedef enum
{
    SENSOR_MODE,    // 感应模式
    LOOP_MODE,      // 环路模式
    CAN_MODE,       // CAN 0-实时 1-队列
    WEAKMAG_MODE,   // 弱磁 模式
    VAGUE_PID_MODE, // 模糊PID
    PVT_MODE,       // PVT 模式
    TRAJ_TYPE,      // 轨迹规划类型

    MOTOR_WIRE_SEQUENCE, // 电机线圈顺序 0-正线 1-反线
    MOTOR_POLEPAIRS,     // 电机转子对数

    FREQ_CURRENT_LOOP,  // 电流环分频系数
    FREQ_SPEED_LOOP,    // 速度环分频系数
    FREQ_POSITION_LOOP, // 位置环分频系数

    // u32
    CAN_ID,

    // float
    f_PWM,           // 这几个参数只可查
    f_CURRENT_LOOP,  // 电流环频率
    f_SPEED_LOOP,    // 速度环频率
    f_POSITION_LOOP, // 位置环频率

    THETA_OFFSET, // 角度补偿
    MOTOR_RS,
    MOTOR_LS,
    MOTOR_Psif,
    MOTOR_Ke, // 反电动势常数
    MOTOR_J,  // 转动惯量
    MOTOR_B,  // 摩擦系数

    Kp_CURRENT,  // 电流环比例
    Ki_CURRENT,  // 电流环积分
    Kp_WEAKMAG,  // 弱磁环比例
    Ki_WEAKMAG,  // 弱磁环积分
    Kp_SPEED,    // 速度环比例
    Ki_SPEED,    // 速度环积分
    Kp_POSITION, // 位置环比例
    Ki_POSITION, // 位置环积分
    Kd_POSITION, // 位置环微分

    LIMIT_CURRENT,      // 电流限幅
    LIMIT_SPEED,        // 速度限幅
    LIMIT_POSITION_min, // 位置限幅
    LIMIT_POSITION_max, // 位置限幅
    TOLERANCE_TIME,     // 容忍时间
    TOLERANCE_VOLTAGE,  // 电压容忍度
    TOLERANCE_CURRENT,  // 电流容忍度
    TOLERANCE_SPEED,    // 速度容忍度
    TOLERANCE_POSITION, // 位置容忍度

    TRAJ_MAX_RATE,  // 轨迹规划最大变化率
    TRAJ_MAX_ACC,   // 轨迹规划最大加速度
    TRAJ_MAX_JERK,  // 轨迹规划最大加加速度
    TRAJ_TOLERANCE, // 轨迹规划容差

    COUNT_PARAM
} eParameter;

typedef struct
{
    // u8类型参数
    u8 none_flag;
    u8 sw_canqueue;
    u8 sw_weakmag;
    u8 sw_vague_pid;
    u8 sw_pvt;
    u8 traj_type;
    u8 sensor_mode;
    u8 run_mode;

    u8 motor_wire_sequence; // 电机线圈顺序 0-正线 1-反线
    u8 motor_polepairs;

    u8 freq_current_loop;  // 电流环分频系数
    u8 freq_speed_loop;    // 速度环分频系数
    u8 freq_position_loop; // 位置环分频系数
    // u32类型参数
    u32 can_id;

    // float类型参数
    float f_pwm; // 这几个参数只可查
    float f_current_loop;
    float f_speed_loop;
    float f_position_loop;

    float theta_offset;
    float motor_rs;
    float motor_ls;
    float motor_psif;
    float motor_ke;
    float motor_j;
    float motor_b;
    float kp_current;
    float ki_current;
    float kp_weakmag;
    float ki_weakmag;
    float kp_speed;
    float ki_speed;
    float kp_position;
    float ki_position;
    float kd_position;
    float limit_current;
    float limit_omega;
    float limit_position_min;
    float limit_position_max;
    float tolerance_time;
    float tolerance_voltage;
    float tolerance_current;
    float tolerance_speed;
    float tolerance_position;

    float traj_max_rate;
    float traj_max_acc;
    float traj_max_jerk;
    float tolerance;
} tParameter;

extern tParameter g_Param;

void fParamSet(eParameter para, u8 *value);
void fParamGet(eParameter para, u8 *value, u8 *len);
bool fParamSave();     // 一键保存
void fParamErase();    // 一键擦除
void fParamWriteFOC(); // 一键写入
bool fParamInit();

#endif // __PARAMETER_MANAGER_H__