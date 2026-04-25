#ifndef __PARAMETER_MANAGER_H__
#define __PARAMETER_MANAGER_H__

#include "main.h"

#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000

typedef enum
{
    SENSOR_MODE,    // 感应模式
    LOOP_MODE,      // 环
    CAN_MODE,       // CAN 0-实时 1-队列 2-实时有反馈 3-队列有反馈
    VAGUE_PID_MODE, // 模糊PID
    PVT_MODE,       // PVT 模式 0-关闭 1-PV 2-PT
    TRAJ_TYPE,      // 轨迹规划器类型

    MOTOR_POLEPAIRS, // 电机极对数

    // u32
    CAN_ID,

    THETA_OFFSET, // 角度补偿
    MOTOR_KV,
    MOTOR_RS,
    MOTOR_Ld,
    MOTOR_Lq,
    MOTOR_Psif,
    MOTOR_Ke, // 反电动势常数
    MOTOR_J,  // 转动惯量
    MOTOR_B,  // 摩擦系数

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
    TOLERANCE_LIMIT,    // 超限容忍度

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
    u8 sw_vague_pid;
    u8 sw_pvt;
    u8 traj_type;
    u8 sensor_mode;
    u8 run_mode;

    u8 motor_polepairs;
    bool theta_elec_offset;
    bool forward_dir;

    // u32类型参数
    u32 can_id;

    // float类型参数

    float theta_offset;
    float motor_kv;
    float motor_rs;
    float motor_ld;
    float motor_lq;
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
    float tolerance_limit;

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