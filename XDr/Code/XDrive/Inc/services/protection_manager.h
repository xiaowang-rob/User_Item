#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"

typedef enum
{
    NO_FAULT,
    MOTOR_ERROR,
    OVER_CURRENT,
    OVER_VOLTAGE,
    UNDER_VOLTAGE,
    OVER_TEMPERATURE,
    CAN_INIT_FAULT,
    CAN_COMMUNICATION_FAULT,
    ENCODER_MAG_WEAK,
    ENCODER_COMMUNICATION_FAULT,
} fault_e;

typedef struct
{
    fault_e fault;
    float maxcurrent;
    float maxspeed;
    float minposition;
    float maxposition;
    float tolerance_time;
    float tolerance_voltage;
    float tolerance_current;
    float tolerance_speed;
    float tolerance_position;
    bool serious_fault;
    bool warning_fault;
    bool log_done;
} protection_manager_t;
extern protection_manager_t g_protection_manager;

void protection_manager_init(float maxcurrent, float max_speed, float min_position, float max_position,
                             float tolerance_time, float tolerance_voltage, float tolerance_current, float tolerance_speed,
                             float tolerance_position);

void protection_manager_clear_fault();
void protection_manager_run();
fault_e GET_Protect_fault();

#endif /* __PROTECTION_MANAGER_H */