#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"
#include "port_mapping.h"
typedef enum
{
    NO_FAULT,
    MOTOR_FAULT,
    OVER_CURRENT,
    OVER_VOLTAGE,
    UNDER_VOLTAGE,
    OVER_SPEED,
    OVER_POSITION,
    CAN_INIT_FAULT,
    CAN_COMMUNICATION_FAULT,
} fault_e;
typedef enum
{
    NO_WARNING,
    OVER_TEMPERATURE,
    ENCODER_OFFLINE,
    ENCODER_COMMUNICATION_ERROR,
    ENCODER_WEAK_MAG,
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
fault_e GET_Protect_fault();

#endif /* __PROTECTION_MANAGER_H */