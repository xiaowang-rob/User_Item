#ifndef __FOC_CORE_H
#define __FOC_CORE_H

#include "bsp.h"
#include "parameter_manager.h"
#include "trajectory.h"

#include "protocol.h"

typedef struct
{
    float cur_filter_alpha;   // 电流滤波系数
    float speed_filter_alpha; // 速度滤波系数
    eSensorMode sensor_mode;
    eRunMode run_mode;
    u8 pvt_mode;
    eTrajType trajectory_mode;
} tFOC_Mode;

typedef struct
{
    float udc;
    float iu_im, iv_im, iw_im;
    float iu, iv, iw;
    float ialpha, ibeta;
    float theta_elec;
    float theta_mech;
    float iq_ref;
    float id_ref;
    float iq_fb, id_fb;
    float ud, uq;
    float ualpha, ubeta;
    float ualpha_hfi, ubeta_hfi;
    float rpm_ref;
    float rpm_fb;
    float pos_ref;
    float pos_fb;
    float tau_ref;
} tFOC_val;

typedef struct
{
    float mech_offset; // 机械偏移角度
    float rs;          // 定子电阻
    float ld;          // 定子电感
    float lq;
    float psi_f;         // 永磁体磁链
    float ke;            // 反电动势常数
    float j;             // 转动惯量
    float b;             // 摩擦系数
    u8 pole_pairs;       // 极对数
    bool forward_dir;    // 正转方向
    bool elec_pi_offset; // 电角度180°偏差
} tMotor;

typedef struct
{
    tMotor *motor;
    tFOC_Mode *foc_mode;
    tFOC_val *foc_val;
} tFOC_Core;

extern tFOC_Core g_foc_core;

// TODO(xdr): 命名规范说明 — fFoc 前缀应改为 foc_
// VESC风格: <模块>_<动作>，全小写下划线
//   fFocCoreInit()      → foc_core_init()
//   fFocValueUpdate()   → foc_value_update()
//   fFocSetTargetValue() → foc_set_target()
//   fFocSetRunMode()    → foc_set_run_mode()
// 类似的: fFilterReset() → filter_reset(), fSetThetaOffset() → foc_set_theta_offset()


void fFocCoreInit();
void fFocParamUpdate(tParameter *param);
void fFocCoreReset();

void fFocValueUpdate();
void fFocMainLoopTask();
bool fFocShutdown();
bool fAutoCalibrationUpdate();

// 辅助整定 函数
void fFilterReset();
void fFocSetUalphaBeta(float Ualpha, float Ubeta);
void fFocSetIdIq(float id, float iq);
void fSetThetaOffset(float thetaoffset);

// 主要函数

void fFocSetTargetValue(float *value);
void fFocSetSensorMode(eSensorMode mode);
void fFocSetRunMode(eRunMode mode);
void fFocSetZeroPos();
void fFocSetLimitPos();
#endif // __FOC_CORE_H