#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "bsp.h"
#include "parameter_manager.h"
#include "trajectory.h"

#include "protocol_defs.h"

typedef struct
{
    float cur_fiter_alpha;   // 电流滤波系数
    float speed_fiter_alpha; // 速度滤波系数
    eSensorMode sensor_mode;
    eRunMode runmode;
    u8 pvt_mode;
    eTrajType trajectory_mode;
} tFOC_Mode;

typedef struct
{
    float Iu_im, Iv_im, Iw_im;
    float Iu, Iv, Iw;
    float Ialpha, Ibeta;
    float theta_elec;
    float theta_mech;
    float iq_ref;
    float id_ref;
    float iq_fb, id_fb;
    float ud, uq;
    float Ualpha, Ubeta;
    float Ualpha_hfi, Ubeta_hfi;
    float rpm_ref;
    float rpm_fb;
    float pos_ref;
    float pos_fb;
} tFOC_val;

typedef struct
{
    float Udc;         // 直流母线电压
    float mech_offect; // 机械偏移角度
    float Rs;          // 定子电阻
    float Ld;          // 定子电感
    float Lq;
    float Psi_f;         // 永磁体磁链
    float Ke;            // 反电动势常数
    float J;             // 转动惯量
    float B;             // 摩擦系数
    u8 pole_pairs;       // 极对数
    bool forward_dir;    // 正转方向
    bool elec_PI_offset; // 电角度180°偏差
} tMotor;

typedef struct
{
    tMotor *motor;
    tFOC_Mode *foc_mode;
    tFOC_val *foc_val;
} tFOC_Core;

extern tFOC_Core foc_core;

void fFOC_CoreInit();
void fFOC_ParamUpdate(tParameter param);
void fFOC_CoreReset();

void fFOC_ValueUpdate();
void fFOC_MainLoopTask();
bool fFOC_Shutdown();
bool fAutoCalibrationUpdate();

// 辅助整定 函数
void fFOC_SetUalphaBeta(float Ualpha, float Ubeta);
void fFOC_SetIdIq(float id, float iq);
void fSetThetaOffset(float thetaoffset);

// 主要函数

void fFOC_SetTargetValue(float *value);
void fFOC_SetSensorMode(eSensorMode mode);
void fFOC_SetRunMode(eRunMode mode);
void fFOC_SetZeroPOS();
void fFOC_SetLimitPOS();
#endif // __FOC_CORE_H