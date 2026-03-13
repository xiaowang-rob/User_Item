#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "main.h"
#include "stdbool.h"
#include "parameter_manager.h"
#include "trajectory.h"
typedef enum
{
    ENCODER_CONTROL,    // 有感控制
    SENSORLESS_CONTROL, // 无感控制 HFI+SMO
    MERGE_CONTROL,      // 混合控制
} eSensorMode;

typedef enum
{
    CURRENT_MODE,
    SPEED_MODE,
    POSITION_MODE, // 增量式控制
    OPEN_LOOP,
} eRunMode;

typedef struct
{
    eSensorMode sensor_mode;
    eRunMode runmode;
    u8 pvt_mode;
    u8 weak_mag;
    eTrajType trajectory_mode;
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
    float ud, uq;
    float Ualpha, Ubeta;
    float omega_ref;
    float omega_fb;
    float pos_ref;
    float pos_fb;
} tFOC_val;

typedef struct
{
    float Udc;          // 直流母线电压
    float offset_angle; // 偏移角度
    float Rs;           // 定子电阻
    float Ld;           // 定子电感
    float Lq;
    float Psi_f;        // 永磁体磁链
    float Ke;           // 反电动势常数
    float J;            // 转动惯量
    float B;            // 摩擦系数
    bool Wire_sequence; // 线序 true-正线序 false-反线序
    u8 pole_pairs;      // 极对数
} tMotor;

typedef struct
{
    tMotor *motor;
    tFOC_Mode *foc_mode;
    tFOC_val *foc_val;
} tFOC_Core;

extern tFOC_Core foc_core;

void fFOC_CoreInit();
void fFOC_CoreReset();

void fFOC_ValueUpdate();
void fFOC_MainLoopTask();
bool fFOC_Shutdown();
bool fAutoCalibrationUpdate();

// 辅助整定 函数
void fFOC_SetUalphaBeta(float Ualpha, float Ubeta);
void fFOC_SetIdIq(float id, float iq);
void fSetThetaOffset(float thetaoffset);
void fSetWireSequence(bool wire_sequence);

// 主要函数

void fFOC_SetTargetValue(float *value);
void fFOC_SetSensorMode(eSensorMode mode);
void fFOC_SetRunMode(eRunMode mode);

#endif // __FOC_CORE_H