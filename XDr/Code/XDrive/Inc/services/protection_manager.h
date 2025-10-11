#ifndef __PROTECTION_MANAGER_H
#define __PROTECTION_MANAGER_H

#include "main.h"

typedef enum
{
    NO_FAULT,
    OVER_CURRENT,
    OVER_VOLTAGE,
    UNDER_VOLTAGE,
    OVER_TEMPERATURE,
    CAN_HARD_FAULT,
    CAN_COMMUNICATION_FAULT,
    ENCODER_MAG_WEAK,
    ENCODER_COMMUNICATION_FAULT,
} fault_t;

typedef struct
{
    fault_t fault;
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
    bool clear_fault;
    bool log_done;
} protection_manager_t;

void protection_manager_run();
fault_t GET_Protect_fault();
#endif /* __PROTECTION_MANAGER_H */