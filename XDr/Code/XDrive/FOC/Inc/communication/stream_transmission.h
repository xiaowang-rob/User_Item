#ifndef __STREAM_TRANSMISSION_H
#define __STREAM_TRANSMISSION_H

#include "main.h"
#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000

#define MODE_LOAD_block 0
#define MODE_LOAD_sector 1
#define MODE_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000
// 反馈参数--流式数据
typedef enum
{
    STATUS, // 四个 系统状态 FOC状态 错误 警告
    TEMPERATURE,
    VBUS,
    VOLTAGE_U,
    VOLTAGE_V,
    VOLTAGE_W,
    VOLTAGE_q,
    VOLTAGE_d,

    CURRENT_U,
    CURRENT_V,
    CURRENT_W,
    CURRENT_q,
    CURRENT_d,
    CURRENT_q_ref,
    CURRENT_d_ref,

    SPEED,
    SPEED_con,
    SPEED_ref,

    THETA_elec,
    THETA_mech,
    THETA_mech_con,
    THETA_mech_ref,
} Data_stream_e;

typedef enum
{
    SW_CANQUEUE,       // CAN队列开关
    SW_WEAKMAG,        // 弱磁开关
    SW_FAN,            // 风扇
    SW_TLC,            // 高温限制电流
    SW_CLS,            // 电流限制速度
    SW_VAGUE_PID,      // 模糊PID
    SW_PVT,            // PVT 模式
    FOC_RUN_MODE,      // 运行模式
    FOC_LOOP_MODE,     // 环路模式
    FOC_AUTOTUNE_MODE, // 自动调参模式
} Mode_e;
// 控制静态参数
typedef struct
{
    bool None_flag;
    bool canqueue;
    bool weakmag;
    bool fan;
    bool tls; // 温度限制电流
    bool cls; // 电流限制速度
    bool vague_pid;
    bool pvt;
    u8 foc_run_mode;      // FOC_RUN_MODE
    u8 foc_loop_mode;     // FOC_LOOP_MODE
    u8 foc_autotune_mode; // FOC_AUTOTUNE_MODE
} Mode_t;
extern Mode_t g_foc_mode;

// 读取数据流
void stream_data_get(Data_stream_e stream, float *data);
// 设置模式
void mode_set(Mode_e mode, u8 *data);
// 读取模式
void mode_ask(Mode_e mode, u8 *data);
// 设置单个参数

// 保存参数
bool parameter_save();
// 擦除参数
void parameter_erase();
// 保存模式
bool mode_save();
// 擦除模式
void mode_erase();

// 初始化
bool parameter_mode_init();
void STATUS_get(u8 *foc_status, u8 *fault);
#endif