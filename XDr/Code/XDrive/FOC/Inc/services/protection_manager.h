#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"
#include "port_mapping.h"
typedef enum
{
    NO_FAULT,
    TUNING_TIMEOUT,          // 整定超时
    POLE_PAIRS_MISMATCH,     // 极对数不匹配
    MOTOR_PARAM_FAULT,       // 电机参数异常
    OVER_VOLTAGE,            // 过压
    LOW_VOLTAGE,             // 低电压
    OVER_CURRENT,            // 过流
    CAN_INIT_FAULT,          // CAN初始化失败
    CAN_COMMUNICATION_FAULT, // CAN通信失败

} fault_e;
typedef enum
{
    NO_WARNING,
    OVER_TEMPERATURE,
    OVER_SPEED,
    OVER_POSITION,
    ENCODER_OFFLINE,   // 无编码器
    ENCODER_COM_ERROR, // 编码器通信错误
    ENCODER_WEAK_MAG,  // 编码器磁场弱
} Warning_e;
typedef struct
{
    u8 temp_u;
    u8 temp_v;
    u8 temp_w;
    float temperature;
    fault_e fault;
    Warning_e warning;
    bool fault_flag;
    bool warning_flag;
    bool log_done;
    float maxcurrent;
    float maxomega;
    float minposition;
    float maxposition;
    float tolerance_time;
    float tolerance_voltage;
    float tolerance_current;
    float tolerance_speed;
    float tolerance_position;

    communication_state_t *com_state;
    Drive_state_t *drive_state;
} protection_manager_t;
extern protection_manager_t g_pro_manager;

void protection_manager_init();

void protection_manager_reset();
void protection_manager_run();

#endif /* __PROTECTION_MANAGER_H */