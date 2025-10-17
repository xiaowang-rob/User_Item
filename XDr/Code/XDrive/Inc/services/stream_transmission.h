#ifndef __STREAM_TRANSMISSION_H
#define __STREAM_TRANSMISSION_H

#include "main.h"
#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000
// 反馈参数--流式数据
typedef enum
{
    VBUS,
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
} Data_stream_e;

// 控制静态参数
typedef enum
{
    CAN_ID,
    CAN_QUEUE, // 开关
    WEAK_MAG,  // 开关
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
    FOC_AUTOTUNE_MODE,  // 自动调参模式
    f_CURRENT_LOOP,     // 电流环频率
    f_SPEED_LOOP,       // 速度环频率
    f_POSITION_LOOP,    // 位置环频率
    Kp_CURRENT,         // 电流环比例
    Ki_CURRENT,         // 电流环积分
    Kp_WEAKMAG,         // 弱磁环比例
    Ki_WEAKMAG,         // 弱磁环积分
    Kp_SPEED,           // 速度环比例
    Ki_SPEED,           // 速度环积分
    Kp_POSITION,        // 位置环比例
    Ki_POSITION,        // 位置环积分
    Kd_POSITION,        // 位置环微分
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
} Parameter_e;

typedef struct
{
    u32 None_flag;
    u32 can_id;               // CAN_ID
    u32 can_queue;            // CAN_QUEUE
    u32 weak_mag;             // WEAK_MAG
    float theta_offset;       // THETA_OFFSET
    u32 motor_polepairs;      // MOTOR_POLEPAIRS
    float motor_rs;           // MOTOR_RS
    float motor_ls;           // MOTOR_LS
    float motor_psif;         // MOTOR_Psif
    float motor_j;            // MOTOR_J
    float motor_b;            // MOTOR_B
    float motor_tc;           // MOTOR_TC
    float reduction_ratio;    // REDUCTION_RATIO
    u32 foc_run_mode;         // FOC_RUN_MODE
    u32 foc_loop_mode;        // FOC_LOOP_MODE
    u32 foc_autotune_mode;    // FOC_AUTOTUNE_MODE
    u32 f_current_loop;       // f_CURRENT_LOOP
    u32 f_speed_loop;         // f_SPEED_LOOP
    u32 f_position_loop;      // f_POSITION_LOOP
    float kp_current;         // Kp_CURRENT
    float ki_current;         // Ki_CURRENT
    float kp_weakmag;         // Kp_WEAKMAG
    float ki_weakmag;         // Ki_WEAKMAG
    float kp_speed;           // Kp_SPEED
    float ki_speed;           // Ki_SPEED
    float kp_position;        // Kp_POSITION
    float ki_position;        // Ki_POSITION
    float kd_position;        // Kd_POSITION
    float limit_current;      // LIMIT_CURRENT
    float limit_speed;        // LIMIT_SPEED
    float limit_position_min; // LIMIT_POSITION_min
    float limit_position_max; // LIMIT_POSITION_max
    float tolerance_time;     // TOLERANCE_TIME
    float tolerance_voltage;  // TOLERANCE_VOLTAGE
    float tolerance_current;  // TOLERANCE_CURRENT
    float tolerance_speed;    // TOLERANCE_SPEED
    float tolerance_position; // TOLERANCE_POSITION
    float startup_pos_grad;   // STARTUP_POS_GRAD
    float startup_spe_grad;   // STARTUP_SPE_GRAD
    float align_current;      // ALIGN_CURRENT
    float align_time;         // ALIGN_TIME
    float open_loop_current;  // OPEN_LOOP_CURRENT
    float open_loop_speed;    // OPEN_LOOP_SPEED
    float change_loop_speed;  // CHANGE_LOOP_SPEED
} Parameter_t;
extern Parameter_t g_foc_parameters;

void stream_data_get(Data_stream_e stream, float *data);
void parameter_set(Parameter_e parameter, u32 *data);
void all_parameters_set(u32 *data);
void parameter_ask(Parameter_e parameter, u32 *data);
void all_parameters_ask(u32 *data, u8 *len);
void parameter_apply();
bool parameter_save();
void parameter_erase();
bool parameter_init();
void CONTROL_value_update(float *data);
void CONTROL_mode_updata(u8 mode);
void STATUS_get(u8 *foc_status, u8 *fault);
#endif