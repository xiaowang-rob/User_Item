#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "main.h"
#include "stdbool.h"
#include "parameter_manager.h"
typedef enum
{
    ENCODER_CONTROL,
    SENSORLESS_CONTROL,
    SVPWM_CONTROL,
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


typedef struct
{
    float Iu, Iv, Iw;
    float Ialpha, Ibeta;
    float theta_elec;
    float theta_mech;
    float iq_ref;
    float id_ref;
    float iq_fb, id_fb;
    float omega_openloop;
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

typedef struct
{
    float Udc;          // 直流母线电压
    float pole_pairs;   // 极对数
    float offset_angle; // 偏移角度
    float Rs;           // 定子电阻
    float Ls;           // 定子电感
    float Psi_f;        // 永磁体磁链
    float Ke;           // 反电动势常数
    float J;            // 转动惯量
    float B;            // 摩擦系数
} Motor_t;

void foc_core_init();
void foc_core_reset();

void FOC_PREPARE();
void FOC_RUN();
bool SHUTDOWM();
bool auto_calibration_update();

void FOC_SET_OMEGA_con(float value);
void FOC_SET_VER_VALUE(float *value);
void FOC_SET_LOOPMODE(LOOP_mode_e mode);
void FOC_SET_RUNMODE(RUN_mode_e mode);

FOC_mode_t *FOC_GET_MODE_adr();
FOC_val_t *FOC_GET_VAL_adr();
startup_mechine_t *FOC_GET_STARTUP_adr();
Motor_t *get_motor_adr();

#endif // __FOC_CORE_H