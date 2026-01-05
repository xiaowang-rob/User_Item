#ifndef __PARAMETER_MANAGER_H__
#define __PARAMETER_MANAGER_H__

#include "main.h"

#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000

typedef enum
{
    SW_CANQUEUE,   // CAN队列开关
    SW_WEAKMAG,    // 弱磁开关
    SW_FAN,        // 风扇
    SW_VAGUE_PID,  // 模糊PID
    SW_PVT,        // PVT 模式
    FOC_MODE,      // 运行模式
    LOOP_MODE,     // 环路模式
    AUTOTUNE_MODE, // 自动调参模式

    MOTOR_POLEPAIRS, // 电机转子对数

    U8_COUNT,
    // u32
    CAN_ID,

    f_CURRENT_LOOP,  // 电流环频率
    f_SPEED_LOOP,    // 速度环频率
    f_POSITION_LOOP, // 位置环频率

    U32_COUNT,
    // float
    THETA_OFFSET, // 角度补偿
    MOTOR_RS,
    MOTOR_LS,
    MOTOR_Psif,
    MOTOR_J, // 转动惯量
    MOTOR_B, // 摩擦系数

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

    STARTUP_POS_GRAD,  // 启动位置斜率
    STARTUP_SPE_GRAD,  // 启动速度斜率
    ALIGN_CURRENT,     // 对齐电流
    ALIGN_TIME,        // 对齐时间
    OPEN_LOOP_CURRENT, // 开环电流
    OPEN_LOOP_SPEED,   // 开环速度
    CHANGE_LOOP_SPEED, // 切环速度

    FLOAT_COUNT,
} Parameter_e;

typedef struct
{
    u8 u8_data[U8_COUNT];
    u32 u32_data[U32_COUNT - U8_COUNT - 1];
    float float_data[FLOAT_COUNT - U32_COUNT - 1];
    u32 none_flag;
} Parameter_t;

void Param_set(Parameter_e para, u8 *value);
void Param_get(Parameter_e para, u8 *value, u8 *len);
bool Param_save();      // 一键保存
void Param_erase();     // 一键擦除
bool Param_write_foc(); // 一键写入
bool Param_init();

#endif // __PARAMETER_MANAGER_H__