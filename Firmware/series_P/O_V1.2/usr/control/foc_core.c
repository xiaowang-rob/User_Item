#include "foc_core.h"
#include "svpwm.h"
#include "math_fast.h"
#include "device.h"
#include "string.h"
#include "smo.h"
#include "tune.h"
#include "loop_control.h"
#include "filter.h"
#include "protection_manager.h"
#include "hfi.h"
#include "usr_config.h"

#include "bsp_adc.h"

static float sin_theta_e = 0.0f;
static float cos_theta_e = 0.0f;

tFOC_Mode g_foc_mode = {0};
tFOC_val g_foc_val = {0};
tMotor g_motor = {0};

tFOC_Core g_foc_core = {.foc_mode = &g_foc_mode, .foc_val = &g_foc_val, .motor = &g_motor};

/* 滤波器实例 */
static tFirstOrderLagFilter _omega_filter;

static tFirstOrderLagFilter _i_u_filter;
static tFirstOrderLagFilter _i_v_filter;
static tFirstOrderLagFilter _i_w_filter;

// 启动器初始化
static void _TrajectoryInit(tParameter *param)
{
    tTraj_Config traj_cfg;
    traj_cfg.max_rate = param->traj_max_rate;
    traj_cfg.max_acc = param->traj_max_acc;
    traj_cfg.max_jerk = param->traj_max_jerk;
    traj_cfg.tolerance = param->tolerance;
    traj_cfg.type = param->traj_type;
    fTraj_Init(traj_cfg);
}

// 电机参数初始化
static void _MotorInit(tParameter *param)
{
    g_motor.mech_offect = param->theta_offset;
    g_motor.pole_pairs = param->motor_polepairs;
    g_motor.elec_pi_offset = param->theta_elec_offset;
    g_motor.forward_dir = param->forward_dir;
    g_motor.rs = param->motor_rs;
    g_motor.ld = param->motor_ld;
    g_motor.lq = param->motor_lq;
    g_motor.psi_f = param->motor_psif;
    g_motor.ke = param->motor_ke;
    g_motor.j = param->motor_j;
    g_motor.b = param->motor_b;
}

// 滤波器初始化
static void _FilterInit(tParameter *param)
{
    if (param->cur_fiter_alpha <= 0.01f || param->cur_fiter_alpha >= 1)
        param->cur_fiter_alpha = 0.4f; // 默认值，确保在合理范围内
    if (param->speed_fiter_alpha <= 0.01f || param->speed_fiter_alpha >= 1)
        param->speed_fiter_alpha = 0.2f; // 默认值，确保在合理范围内
    fFirstOrderLagInit(&_i_u_filter, param->cur_fiter_alpha, 0);
    fFirstOrderLagInit(&_i_v_filter, param->cur_fiter_alpha, 0);
    fFirstOrderLagInit(&_i_w_filter, param->cur_fiter_alpha, 0);

    fFirstOrderLagInit(&_omega_filter, param->speed_fiter_alpha, 0);
}

void fFilterReset()
{
    _FilterInit(&g_Param);
}
// FOC参数更新（外部调用，参数修改后需调用）
void fFocParamUpdate(tParameter *param)
{
    BSP_SetAdcCurrentOffset(param->adc_U_zero_offset, param->adc_V_zero_offset, param->adc_W_zero_offset);
    fEncoder_Init((eEncoderChip)param->encoder_chip);
    _MotorInit(param);
    BSP_AdcGetVoltage(&g_foc_val.udc);
    fSvpwmInit(g_foc_val.udc);
    fLoopControlInit(param, g_foc_val.udc);

    _TrajectoryInit(param);

    fSmoInit(&g_motor);
    fFocSetSensorMode(param->sensor_mode);
    g_foc_mode.run_mode = param->run_mode;
    g_foc_mode.pvt_mode = param->sw_pvt;
    fFocCoreReset();
}
// FOC核心初始化
void fFocCoreInit(void)
{
    fFocParamUpdate(&g_Param); // 参数加载
    fHfiInit();
    _FilterInit(&g_Param);
}

// 重置FOC中间变量
static void _FocValReset(void)
{
    g_foc_val.id_ref = 0;
    g_foc_val.iq_ref = 0;
    g_foc_val.rpm_ref = 0;
    g_foc_val.pos_ref = 0;
    g_foc_val.ualpha = 0;
    g_foc_val.ubeta = 0;
    g_foc_val.ualpha_hfi = 0;
    g_foc_val.ubeta_hfi = 0;
    g_foc_val.ud = 0;
    g_foc_val.uq = 0;
}

// FOC 复位
void fFocCoreReset(void)
{
    fHfiResetInitialPosition();
    fMotorParamTuneReset();
    fSmoReset();
    _FocValReset();
    // 刷新电压，确保参数更新后电压环能正确工作
    BSP_AdcGetVoltage(&g_foc_val.udc);
    fLoopReset(g_foc_val.udc);
    fSvpwmInit(g_foc_val.udc);

    g_foc_val.pos_fb = fGetEncoderAngle_INC();
    g_foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
    if (g_foc_mode.run_mode == POSITION_MODE)
        fTraj_Reset(g_foc_val.pos_fb);
    else if (g_foc_mode.run_mode == SPEED_MODE)
        fTraj_Reset(g_foc_val.rpm_fb);
}

// 电流重构：根据扇区将线电流转换为相电流
static inline void _CurrentReconstruction(void)
{
    float ui, vi, wi;
    BSP_AdcGetCurrent(&ui, &vi, &wi);

    switch (fSvpwmGetSector())
    {
    case 1:
    case 6:
        g_foc_val.iu_im = vi + wi;
        g_foc_val.iv_im = -vi;
        g_foc_val.iw_im = -wi;
        break;
    case 2:
    case 3:
        g_foc_val.iu_im = -ui;
        g_foc_val.iv_im = ui + wi;
        g_foc_val.iw_im = -wi;
        break;
    case 4:
    case 5:
        g_foc_val.iu_im = -ui;
        g_foc_val.iv_im = -vi;
        g_foc_val.iw_im = ui + vi;
        break;
    default:
        g_foc_val.iu_im = ui;
        g_foc_val.iv_im = vi;
        g_foc_val.iw_im = wi;
        break;
    }
    g_foc_val.iu = fFirstOrderLagFilter(&_i_u_filter, g_foc_val.iu_im);
    g_foc_val.iv = fFirstOrderLagFilter(&_i_v_filter, g_foc_val.iv_im);
    g_foc_val.iw = fFirstOrderLagFilter(&_i_w_filter, g_foc_val.iw_im);

    fClarkTransform(g_foc_val.iu, g_foc_val.iv, g_foc_val.iw, &g_foc_val.ialpha, &g_foc_val.ibeta);
}

void fFocValueUpdate(void)
{
    fFrequencyDivisionUpdate();
    BSP_AdcGetVoltage(&g_foc_val.udc);
    _CurrentReconstruction();

    switch (g_foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 获取编码器数据
        g_foc_val.theta_mech = fGetEncoderAngle_ABS();
        g_foc_val.theta_elec = (g_foc_val.theta_mech - g_motor.mech_offect) * g_motor.pole_pairs * (g_motor.forward_dir ? 1 : -1) + (g_motor.elec_pi_offset ? 180 : 0);
        g_foc_val.theta_elec = fNormalizeAngle_0_360(g_foc_val.theta_elec);

        if (!g_loop_con.fd.speed_update)
            break;
        g_foc_val.pos_fb = fGetEncoderAngle_INC();
        g_foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
        break;

    case SENSORLESS_CONTROL: // todo:这里只获取和处理无感观测器的数据

        g_foc_val.theta_elec = fHfiGetThetaElec();
        g_foc_val.rpm_fb = fHfiGetOmegaElec() / g_motor.pole_pairs;
        // todo:做累加电角度才能反馈真实的机械角速度和位置
        // g_foc_val.theta_mech =
        // g_foc_val.pos_fb =

        // g_foc_val.rpm_fb = fHfiGetOmegaElec() / g_motor.pole_pairs;
        //        g_foc_val.rpm_fb = fHfiGetOmegaElec(); // 先让他等于电角速度
        //        g_foc_val.rpm_fb = FABSF(g_foc_val.rpm_fb) < 0.1 ? 0 : g_foc_val.rpm_fb;
        break;
    case MERGE_CONTROL:
        g_foc_val.theta_mech = fGetEncoderAngle_ABS();
        g_foc_val.theta_elec = (g_foc_val.theta_mech - g_motor.mech_offect) * g_motor.pole_pairs * (g_motor.forward_dir ? 1 : -1) + (g_motor.elec_pi_offset ? 180 : 0);
        g_foc_val.theta_elec = fNormalizeAngle_0_360(g_foc_val.theta_elec);

        g_foc_val.pos_fb = fGetEncoderAngle_INC();
        g_foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
        g_foc_val.rpm_fb = FABSF(g_foc_val.rpm_fb) < 0.1 ? 0 : g_foc_val.rpm_fb;

        break;
    default:
        break;
    }
    arm_sin_cos_f32(g_foc_val.theta_elec, &sin_theta_e, &cos_theta_e);
    fParkTransform(g_foc_val.ialpha, g_foc_val.ibeta, sin_theta_e, cos_theta_e, &g_foc_val.id_fb, &g_foc_val.iq_fb);
}
// 使能后执行：按模式运行对应控制环
void fFocMainLoopTask(void)
{
    if (g_foc_mode.sensor_mode >= SENSORLESS_CONTROL)
    {
        fHfiStep(g_foc_val.ialpha, g_foc_val.ibeta, &g_foc_val.ualpha_hfi, &g_foc_val.ubeta_hfi);
        // g_smo
        if (!fHfiGetStatus())
        { // todo:使能之后 直接跑电压环
            fHfiDetectInitialPosition(g_foc_val.id_fb, &g_foc_val.ualpha, &g_foc_val.ubeta);
            fSvpwmRun(g_foc_val.ualpha + g_foc_val.ualpha_hfi, g_foc_val.ubeta + g_foc_val.ubeta_hfi);
            fSamplePointCalibration();
            return;
        }
        // fSmoMainLoop(g_foc_val.ualpha, g_foc_val.ubeta, g_foc_val.ialpha, g_foc_val.ibeta);
    }

    switch (g_foc_mode.run_mode)
    {
    case POSITION_MODE:
        if (!g_loop_con.fd.position_update)
            break;

        tTraj_Out traj_out = fTraj_Update(g_loop_con.fd.t_pos);
        g_foc_val.pos_ref = traj_out.value;
        g_foc_val.rpm_ref = fPositionRelLoopUpdate(g_foc_val.pos_ref, g_foc_val.pos_fb);
    case SPEED_MODE:
        if (!g_loop_con.fd.speed_update)
            break;
        if (g_foc_mode.run_mode == SPEED_MODE)
        {
            tTraj_Out traj_out = fTraj_Update(g_loop_con.fd.t_spd);
            g_foc_val.rpm_ref = traj_out.value;
        }
        g_foc_val.iq_ref = fSpeedLoopUpdate(g_foc_val.rpm_ref, g_foc_val.rpm_fb);
        g_foc_val.id_ref = fWeakMagLoopUpdate(g_foc_val.ud, g_foc_val.uq);

    case CURRENT_MODE:
        if (!g_loop_con.fd.current_update)
            break;

        g_foc_val.uq = fCurrentLoopUpdate(g_foc_val.iq_ref, g_foc_val.iq_fb);
        g_foc_val.ud = fMagLoopUpdate(g_foc_val.id_ref, g_foc_val.id_fb);
        fInvParkTransform(g_foc_val.ud, g_foc_val.uq, sin_theta_e, cos_theta_e, &g_foc_val.ualpha, &g_foc_val.ubeta);
        break;

    default: // 开环模式
        break;
    }

    fSvpwmRun(g_foc_val.ualpha + g_foc_val.ualpha_hfi, g_foc_val.ubeta + g_foc_val.ubeta_hfi);
    fSamplePointCalibration();
}

// 设置各环指令值
void fFocSetTargetValue(float *value)
{
    switch (g_foc_mode.run_mode)
    {
    case CURRENT_MODE:
        g_foc_val.iq_ref = value[0];
        g_foc_val.id_ref = value[1];
        break;
    case SPEED_MODE:
        fTraj_SetTarget(value[0]);
        break;
    case POSITION_MODE:
        fTraj_SetTarget(value[0]);
        if (g_foc_mode.pvt_mode)
            fTraj_SetRate(value[1]);
        break;
    default: // IDLE 可以直接设置ualpha和ubeta
        break;
    }
}

// 参数自动校准
bool fAutoCalibrationUpdate(void)
{
    if (TUNE_DONE == fMotorParamTuneUpdate(g_foc_val))
    {
        fFocCoreInit();
        return true;
    }
    return false;
}
// 设置 αβ 电压
void fFocSetUalphaBeta(float Ualpha, float Ubeta)
{
    g_foc_val.ualpha = Ualpha;
    g_foc_val.ubeta = Ubeta;
}
// 设置 dq 电流
void fFocSetIdIq(float id, float iq)
{
    g_foc_val.id_ref = id;
    g_foc_val.iq_ref = iq;
}
// 设置编码器零点偏移
void fSetThetaOffset(float thetaoffset)
{
    g_motor.mech_offect = thetaoffset;
}

// 切换传感模式
void fFocSetSensorMode(eSensorMode mode)
{
    fFocCoreReset();
    g_foc_mode.sensor_mode = mode;
}
// 切换控制模式
void fFocSetRunMode(eRunMode mode)
{
    _FocValReset();
    g_foc_mode.run_mode = mode;
}

// 强制刹车
bool fFocShutdown(void)
{
    if (fabsf(g_foc_val.rpm_fb) < 0.1f)
        return true;
    if (g_foc_mode.run_mode != SPEED_MODE)
        fFocSetRunMode(SPEED_MODE);
    float omega_shutdown = -g_foc_val.rpm_fb * 0.5f;
    fFocSetTargetValue(&omega_shutdown);
    return false;
}
// 设置位置零点
void fFocSetZeroPos()
{
    fSetEncoderAngleZero();
}
// 设置限位位置
void fFocSetLimitPos()
{
    if (g_foc_val.pos_fb > 0)
        g_Param.limit_position_max = g_foc_val.pos_fb;
    else
        g_Param.limit_position_min = g_foc_val.pos_fb;
    fFocCoreInit();
    fProSetLimitPosition(g_Param.limit_position_min, g_Param.limit_position_max);
}
