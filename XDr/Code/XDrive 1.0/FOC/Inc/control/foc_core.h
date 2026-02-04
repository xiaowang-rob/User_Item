#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "main.h"
#include "stdbool.h"
#include "parameter_manager.h"
typedef enum
{
    OPEN_LOOP_CONTROL,
    ENCODER_CONTROL,
    SMO_CONTROL,
    ENCODER_SMO_CONTROL,
} eSensorMode;

typedef enum
{
    VOLTAGE_LOOP,
    CURRENT_LOOP,
    SPEED_LOOP,
    POSITION_ABS_LOOP, // 绝对位置控制(0-360°)
    POSITION_REL_LOOP, // 相对/增量位置控制 会以给定角度（+-float的范围）转到位置

    IDLE,
} eLoopMode;

typedef struct
{
    eSensorMode sensor_mode;
    eLoopMode loop_mode;
    bool pvt_mode;
    bool weak_mag;
} tFOC_Mode;

typedef struct
{
    float Iu, Iv, Iw;
    float Ialpha, Ibeta;
    float theta_elec;
    float theta_mech;
    float iq_ref;
    float id_ref;
    float iq_fb, id_fb;
    float theta_openloop;
    float omega_openloop;
    float ud, uq;
    float Ualpha, Ubeta;
    float omega_ref;
    float omega_con;
    float omega_fb;
    float pos_ref;
    float pos_fb;
    eLoopMode loop_state;
} tFOC_val;

// todo:这个启动得改改
typedef struct
{
    float omega_acc;

    float align_id; // 对齐电流
    u32 current_steps;
    u32 align_steps;
    float openloop_iq; // 开环电流
    float openloop_omega;
    bool change_flag;
    bool align_flag;

} tStartupMechine;

typedef struct
{
    float Udc;            // 直流母线电压
    float offset_angle;   // 偏移角度
    float Rs;             // 定子电阻
    float Ls;             // 定子电感
    float Psi_f;          // 永磁体磁链
    float Ke;             // 反电动势常数
    float J;              // 转动惯量
    float B;              // 摩擦系数
    int8_t Wire_sequence; // 线序 +1-正线序 -1-反线序
    u8 pole_pairs;        // 极对数
} tMotor;

typedef struct
{
    tMotor *motor;
    tFOC_Mode *foc_mode;
    tFOC_val *foc_val;
    tStartupMechine *startup_machine;
} tFOC_Core;

extern tFOC_Core foc_core;

void fFOC_CoreInit();
void fFOC_CoreReset();

void fFOC_MainLoop();
bool fFOC_Shutdown();
bool fAutoCalibrationUpdate();

void fSetThetaOffset(float thetaoffset);
void fSetWireSequence(int wire_sequence);

void fSetOpendLoopTheta(float theta_elec);
void fSetOpendLoopOmega(float omega_elec);

void fFOC_SetOmegaIM(float value);
void fFOC_SetTargetValue(float *value);
void fFOC_SetSensorMode(eSensorMode mode);
void fFOC_SetLoopMode(eLoopMode mode);

#endif // __FOC_CORE_H