#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "main.h"
#include "stdbool.h"

typedef enum
{
    ENCODER_CONTROL,
    SENSORLESS_CONTROL,
} RUN_MODE_e;
typedef enum
{
    CURRENT_LOOP_CONTROL,
    SPEED_LOOP_CONTROL,
    POSITION_ABS_CONTROL, // 绝对位置控制 会以最优方式转到位置
    POSITION_REL_CONTROL, // 相对/增量位置控制 会以给定角度（+-float的范围）转到位置
} LOOP_MODE_e;
typedef enum
{
    FOC_INIT,
    FOC_AUTO_TUNE,
    FOC_IDLE,
    FOC_STARTUP,
    FOC_RUNNING,
    FOC_SHUTDOWN,
    FOC_FAULT
} FOC_STATE_e;

typedef struct
{
    float Iu, Iv, Iw;
    float Ialpha, Ibeta;
    float theta_elec;
    float theta_mech;
    float theta_mech_last;
    float iq_fb, id_fb;
    float Ualpha, Ubeta;
    float omega_fb;
    float pos_fb;
} Monitor_t;
extern Monitor_t g_monitor;

typedef struct
{
    RUN_MODE_e run_mode;
    LOOP_MODE_e loop_mode;
    bool enable;
    float iq_ref;
    float id_ref;
    float omega_ref;
    float omega_con;
    float pos_ref;
    float pos_con;
} foc_core_t;
extern foc_core_t g_foccore;
#endif // __FOC_CORE_H