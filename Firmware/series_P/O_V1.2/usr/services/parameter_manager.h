#ifndef __PARAMETER_MANAGER_H
#define __PARAMETER_MANAGER_H

#include "bsp.h"
#include "protocol.h"

#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000

typedef struct
{
    // u8类型参数
    u8 none_flag;
    u8 encoder_chip;
    u8 sw_canqueue;
    u8 sw_vague_pid;
    u8 sw_pvt;
    u8 traj_type;
    u8 sensor_mode;
    u8 run_mode;

    u8 motor_polepairs;
    bool theta_elec_offset;
    bool forward_dir;

    // u32类型参数
    u32 can_id;

    // float类型参数

    float theta_offset;
    float motor_kv;
    float motor_rs;
    float motor_ld;
    float motor_lq;
    float motor_psif;
    float motor_ke;
    float motor_j;
    float motor_b;

    float kp_speed;
    float ki_speed;
    float kp_position;
    float ki_position;
    float kd_position;
    float kp_MIT;
    float kd_MIT;
    float tff_MIT;
    float tmax_MIT;
    float tune_current;
    float limit_current;
    float limit_omega;
    float limit_position_min;
    float limit_position_max;
    float tolerance_time;
    float tolerance_limit;

    float traj_max_rate;
    float traj_max_acc;
    float traj_max_jerk;
    float tolerance;

    // 不需要上位机改写的参数可以放在这里，避免误改
    float kp_Q;
    float ki_Q;
    float kp_D;
    float ki_D;
    float kp_weakmag;
    float ki_weakmag;

    float cur_filter_alpha;   // 电流滤波系数
    float speed_filter_alpha; // 速度滤波系数
    float adc_U_zero_offset; // ADC零点补偿
    float adc_V_zero_offset; // ADC零点补偿
    float adc_W_zero_offset; // ADC零点补偿
} tParameter;

extern tParameter g_Param;

void fParamSet(eParameter para, u8 *value);
void fParamGet(eParameter para, u8 *value, u8 *len);
bool fParamSave();  // 一键保存
void fParamErase(); // 一键擦除
bool fParamInit();

#endif // __PARAMETER_MANAGER_H