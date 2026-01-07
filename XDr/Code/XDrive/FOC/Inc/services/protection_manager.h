#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"

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
    fault_e fault;
    Warning_e warning;
    bool fault_flag;
    bool warning_flag;
    bool log_done;
    float maxcurrent;
    float maxspeed;
    float minposition;
    float maxposition;
    float tolerance_time;
    float tolerance_voltage;
    float tolerance_current;
    float tolerance_speed;
    float tolerance_position;
    float temp_u;
    float temp_v;
    float temp_w;
    float temperature;
} protection_manager_t;
extern protection_manager_t g_protection_manager;

void protection_manager_init(float maxcurrent, float max_speed, float min_position, float max_position,
                             float tolerance_time, float tolerance_voltage, float tolerance_current, float tolerance_speed,
                             float tolerance_position);

void protection_manager_reset();
void protection_manager_run();
fault_e GET_Protect_fault();

#endif /* __PROTECTION_MANAGER_H */