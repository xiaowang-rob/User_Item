#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "main.h"
#include "stdbool.h"
#include "foc_statemachine.h"
typedef enum
{
    AUTO_TUNE_CONTROL,
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

typedef struct
{
    float Iu, Iv, Iw;
    float Ialpha, Ibeta;
    float theta_elec;
    float theta_mech;
    float theta_mech_last;
    float iq_fb, id_fb;
    float ud, uq;
    float Ualpha, Ubeta;
    float omega_fb;
    float pos_fb;
} Monitor_t;
extern Monitor_t g_monitor;

typedef struct
{
    FOC_STATE_e state;
    RUN_MODE_e run_mode;
    LOOP_MODE_e loop_mode;
    float Reduction_ratio; // 减速比
    float iq_ref;
    float id_ref;
    float omega_ref;
    float omega_con;
    float pos_ref;
    float pos_con;
} foc_core_t;
extern foc_core_t g_foccore;

typedef struct
{
    float pos_gradient;
    float omega_gradient;

    float align_cur;
    u32 current_steps;
    u32 align_steps;

    float openloop_cur;
    float openloop_omega;
    float changeloop_speed;
    bool change_flag;
    bool align_flag;

} startup_mechine_t;

bool foc_core_init();
void svpwm_udc_update();
void foc_core_run();
void foc_disable();
void foc_enable();
bool foc_shutdown();
void foc_loop_mode_change(LOOP_MODE_e mode);
#endif // __FOC_CORE_H