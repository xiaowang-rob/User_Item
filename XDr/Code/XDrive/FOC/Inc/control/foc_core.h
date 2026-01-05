#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "main.h"
#include "stdbool.h"
#include "foc_statemachine.h"
typedef enum
{
    ENCODER_CONTROL,
    SENSORLESS_CONTROL,
} RUN_MODE_e;
typedef enum
{
    VOLTAGE_LOOP,
    CURRENT_LOOP,
    SPEED_LOOP,
    POSITION_ABS_LOOP, // 绝对位置控制(0-360°)
    POSITION_REL_LOOP, // 相对/增量位置控制 会以给定角度（+-float的范围）转到位置
} LOOP_MODE_e;

typedef struct
{
    float Iu, Iv, Iw;
    float Ialpha, Ibeta;
    float theta_elec;
    float theta_mech;
    float iq_ref;
    float id_ref;
    float iq_fb, id_fb;
    float ud, uq;
    float Ualpha, Ubeta;
    float omega_ref;
    float omega_con;
    float omega_fb;
    float pos_ref;
    float pos_con;
    float pos_fb;
} FOC_val_t;

typedef struct
{
    float pos_gradient;
    float omega_gradient;

    float align_ud; // 对齐电压
    u32 current_steps;
    u32 align_steps;

    float openloop_uq; // 开环电压
    float openloop_omega;
    bool change_flag;
    bool align_flag;

} startup_mechine_t;

bool foc_core_init();
void foc_core_reset();
void foc_core_run();
void CONTROL_value_update(float *data);
void CONTROL_mode_updata(u8 mode);
#endif // __FOC_CORE_H