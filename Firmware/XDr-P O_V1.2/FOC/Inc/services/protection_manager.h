#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"
#include "device.h"
#include "port_mapping.h"
typedef enum
{
    NO_FAULT,
    FLASH_OFFLINE,             // 闪存离线
    TUNING_CURRENT_VIBRATION,  // 整定电流震荡
    TUNING_POLEPAIRS_MISMATCH, // 极对数不匹配
    TUNING_MOTOR_LOCKED,       // 电机堵转
    TUNING_RSLS_FAULT,         // RSLS校准失败
    TUNING_ENCODER_FAULT,      // 编码器校准失败
    TUNING_ELECTRI_FAULT,      // 电机电气参数校准失败
    TUNING_MECH_FAULT,         // 电机机械参数校准失败
    OVER_VOLTAGE,              // 过压
    LOW_VOLTAGE,               // 低电压
    OVER_CURRENT,              // 过流
    CAN_INIT_FAULT,            // CAN初始化失败
    CAN_COMMUNICATION_FAULT,   // CAN通信异常

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
    float temperature;
    eFault fault;
    eWarning warning;
    bool fault_flag;
    bool warning_flag;
    float maxcurrent;
    float maxomega;
    float minposition;
    float maxposition;
    float tolerance_time_ms;
    float tolerance_limit;

    tCommunicationState *com_state;
    tDeviceStatus *drive_state;
} tProtectionManager;
extern tProtectionManager g_pro_manager;

// functions
void fProManagerInit();
void fProManagerReset();
void fProManagerMainLoop();

void fProSetLimitPosition(float min_position, float max_position);

#endif /* __PROTECTION_MANAGER_H */