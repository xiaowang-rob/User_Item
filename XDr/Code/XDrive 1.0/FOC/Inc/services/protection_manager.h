#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"
#include "device.h"
#include "port_mapping.h"
typedef enum
{
    NO_FAULT,
    FLASH_OFFLINE,           // 闪存离线
    TUNING_TIMEOUT,          // 整定超时
    POLE_PAIRS_MISMATCH,     // 极对数不匹配
    MOTOR_PARAM_FAULT,       // 电机参数异常
    OVER_VOLTAGE,            // 过压
    LOW_VOLTAGE,             // 低电压
    OVER_CURRENT,            // 过流
    CAN_INIT_FAULT,          // CAN初始化失败
    CAN_COMMUNICATION_FAULT, // CAN通信失败

} eFault;
typedef enum
{
    NO_WARNING,
    OVER_TEMPERATURE,
    OVER_SPEED,
    OVER_POSITION,
    ENCODER_OFFLINE,   // 无编码器 或者 磁场弱
    ENCODER_COM_ERROR, // 编码器通信错误
} eWarning;
typedef struct
{
    u8 temp_u;
    u8 temp_v;
    u8 temp_w;
    float temperature;
    eFault fault;
    eWarning warning;
    bool fault_flag;
    bool warning_flag;
    bool log_done;
    float maxcurrent;
    float maxomega;
    float minposition;
    float maxposition;
    float tolerance_time_ms;
    float tolerance_voltage;
    float tolerance_current;
    float tolerance_speed;
    float tolerance_position;

    tCommunicationState *com_state;
    tDeviceStatus *drive_state;
} tProtectionManager;
extern tProtectionManager g_pro_manager;

// functions
void fProManagerInit();
void fProManagerReset();
void fProManagerMainLoop();

#endif /* __PROTECTION_MANAGER_H */