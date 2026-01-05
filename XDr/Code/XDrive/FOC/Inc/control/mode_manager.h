#ifndef __MODE_MANAGER_H__
#define __MODE_MANAGER_H__

#include "main.h"
#include "foc_statemachine.h"
typedef enum
{
    ENCODER_CONTROL,
    SENSORLESS_CONTROL,
} RUN_mode_e;

typedef enum
{
    VOLTAGE_LOOP,
    CURRENT_LOOP,
    SPEED_LOOP,
    POSITION_ABS_LOOP, // 绝对位置控制(0-360°)
    POSITION_REL_LOOP, // 相对/增量位置控制 会以给定角度（+-float的范围）转到位置
} LOOP_mode_e;

typedef struct
{
    RUN_mode_e run_mode;
    LOOP_mode_e loop_mode;
    bool pvt_mode;
    bool weak_mag;
} FOC_mode_t;

#endif //__MODE_MANAGER_H__