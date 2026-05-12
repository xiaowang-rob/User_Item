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

tFOC_Mode foc_mode = {0};
tFOC_val foc_val = {0};
tMotor Motor = {0};

tFOC_Core foc_core = {.foc_mode = &foc_mode, .foc_val = &foc_val, .motor = &Motor};

/* 滤波器实例 */
static tFirstOrderLagFilter _omega_filter;

static tFirstOrderLagFilter _i_u_filter;
static tFirstOrderLagFilter _i_v_filter;
static tFirstOrderLagFilter _i_w_filter;

// 启动器初始化
static void _trajectory_init(tParameter *param)
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
static void _motor_init(tParameter *param)
{
    Motor.mech_offect = param->theta_offset;
    Motor.pole_pairs = param->motor_polepairs;
    Motor.elec_PI_offset = param->theta_elec_offset;
    Motor.forward_dir = param->forward_dir;
    Motor.Rs = param->motor_rs;
    Motor.Ld = param->motor_ld;
    Motor.Lq = param->motor_lq;
    Motor.Psi_f = param->motor_psif;
    Motor.Ke = param->motor_ke;
    Motor.J = param->motor_j;
    Motor.B = param->motor_b;
}

// 滤波器初始化
static void _filter_init(tParameter *param)
{
    if (param->cur_fiter_alpha <= 0.01f || param->cur_fiter_alpha >= 1)
        param->cur_fiter_alpha = 0.4f; // 默认值，确保在合理范围内
    fFirstOrderLagInit(&_i_u_filter, param->cur_fiter_alpha, 0);
    fFirstOrderLagInit(&_i_v_filter, param->cur_fiter_alpha, 0);
    fFirstOrderLagInit(&_i_w_filter, param->cur_fiter_alpha, 0);

    fFirstOrderLagInit(&_omega_filter, param->cur_fiter_alpha, 0);
}

void fFilter_Reset()
{
    _filter_init(&g_Param);
}
// FOC参数更新（外部调用，参数修改后需调用）
void fFOC_ParamUpdate(tParameter *param)
{
    BSP_SetAdcCurrentOffset(param->adc_U_zero_offset, param->adc_V_zero_offset, param->adc_W_zero_offset);
    fEncoder_Init((eEncoderChip)param->encoder_chip);
    _motor_init(param);
    BSP_AdcGetVoltage(&foc_val.Udc);
    fSvpwmInit(foc_val.Udc);
    fLoopControlInit(param, foc_val.Udc);

    _trajectory_init(param);

    fSMO_Init(&Motor);
    fFOC_SetSensorMode(param->sensor_mode);
    foc_mode.runmode = param->run_mode;
    foc_mode.pvt_mode = param->sw_pvt;
    fFOC_CoreReset();
}
// FOC核心初始化
void fFOC_CoreInit(void)
{
    fFOC_ParamUpdate(&g_Param); // 参数加载
    fHFI_Init();
    _filter_init(&g_Param);
}

// 重置FOC中间变量
static inline void _FocValReset(void)
{
    foc_val.id_ref = 0;
    foc_val.iq_ref = 0;
    foc_val.rpm_ref = 0;
    foc_val.pos_ref = 0;
    foc_val.Ualpha = 0;
    foc_val.Ubeta = 0;
    foc_val.Ualpha_hfi = 0;
    foc_val.Ubeta_hfi = 0;
    foc_val.ud = 0;
    foc_val.uq = 0;
}

// FOC 复位
void fFOC_CoreReset(void)
{
    fHFI_ResetInitialPosition();
    fMotorParamTune_Reset();
    fSMO_Reset();
    _FocValReset();
    // 刷新电压，确保参数更新后电压环能正确工作
    BSP_AdcGetVoltage(&foc_val.Udc);
    fLoopReset(foc_val.Udc);
    fSvpwmInit(foc_val.Udc);

    foc_val.pos_fb = fGetEncoderAngle_INC();
    foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
    if (foc_mode.runmode == POSITION_MODE)
        fTraj_Reset(foc_val.pos_fb);
    else if (foc_mode.runmode == SPEED_MODE)
        fTraj_Reset(foc_val.rpm_fb);
}

// 电流重构：根据扇区将线电流转换为相电流
static inline void _Current_reconstruction(void)
{
    float ui, vi, wi;
    BSP_AdcGetCurrent(&ui, &vi, &wi);

    switch (fSvpwmGetSector())
    {
    case 1:
    case 6:
        foc_val.Iu_im = vi + wi;
        foc_val.Iv_im = -vi;
        foc_val.Iw_im = -wi;
        break;
    case 2:
    case 3:
        foc_val.Iu_im = -ui;
        foc_val.Iv_im = ui + wi;
        foc_val.Iw_im = -wi;
        break;
    case 4:
    case 5:
        foc_val.Iu_im = -ui;
        foc_val.Iv_im = -vi;
        foc_val.Iw_im = ui + vi;
        break;
    default:
        foc_val.Iu_im = ui;
        foc_val.Iv_im = vi;
        foc_val.Iw_im = wi;
        break;
    }
    foc_val.Iu = fFirstOrderLagFilter(&_i_u_filter, foc_val.Iu_im);
    foc_val.Iv = fFirstOrderLagFilter(&_i_v_filter, foc_val.Iv_im);
    foc_val.Iw = fFirstOrderLagFilter(&_i_w_filter, foc_val.Iw_im);

    fClarkTransform(foc_val.Iu, foc_val.Iv, foc_val.Iw, &foc_val.Ialpha, &foc_val.Ibeta);
}

void fFOC_ValueUpdate(void)
{
    fFrequencyDivisionUpdate();
    BSP_AdcGetVoltage(&foc_val.Udc);
    _Current_reconstruction();

    switch (foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 获取编码器数据
        foc_val.theta_mech = fGetEncoderAngle_ABS();
        foc_val.theta_elec = (foc_val.theta_mech - Motor.mech_offect) * Motor.pole_pairs * (Motor.forward_dir ? 1 : -1) + (Motor.elec_PI_offset ? 180 : 0);
        foc_val.theta_elec = fNormalizeAngle_0_360(foc_val.theta_elec);

        if (!loop_con.fd.speed_update)
            break;
        foc_val.pos_fb = fGetEncoderAngle_INC();
        foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
        break;

    case SENSORLESS_CONTROL: // todo:这里只获取和处理无感观测器的数据

        foc_val.theta_elec = fHFI_GetThetaElec();
        foc_val.rpm_fb = fHFI_GetOmegaElec() / Motor.pole_pairs;
        // todo:做累加电角度才能反馈真实的机械角速度和位置
        // foc_val.theta_mech =
        // foc_val.pos_fb =

        // foc_val.rpm_fb = fHFI_GetOmegaElec() / Motor.pole_pairs;
        //        foc_val.rpm_fb = fHFI_GetOmegaElec(); // 先让他等于电角速度
        //        foc_val.rpm_fb = FABSF(foc_val.rpm_fb) < 0.1 ? 0 : foc_val.rpm_fb;
        break;
    case MERGE_CONTROL:
        foc_val.theta_mech = fGetEncoderAngle_ABS();
        foc_val.theta_elec = (foc_val.theta_mech - Motor.mech_offect) * Motor.pole_pairs * (Motor.forward_dir ? 1 : -1) + (Motor.elec_PI_offset ? 180 : 0);
        foc_val.theta_elec = fNormalizeAngle_0_360(foc_val.theta_elec);

        foc_val.pos_fb = fGetEncoderAngle_INC();
        foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
        foc_val.rpm_fb = FABSF(foc_val.rpm_fb) < 0.1 ? 0 : foc_val.rpm_fb;

        break;
    default:
        break;
    }
    fParkTransform(foc_val.Ialpha, foc_val.Ibeta, foc_val.theta_elec, &foc_val.id_fb, &foc_val.iq_fb);
}
// 使能后执行：按模式运行对应控制环
void fFOC_MainLoopTask(void)
{
    if (foc_mode.sensor_mode >= SENSORLESS_CONTROL)
    {
        fHFI_Step(foc_val.Ialpha, foc_val.Ibeta, &foc_val.Ualpha_hfi, &foc_val.Ubeta_hfi);
        // smo
        if (!fHFI_GetStatus())
        { // todo:使能之后 直接跑电压环
            fHFI_DetectInitialPosition(foc_val.Ialpha, foc_val.Ibeta, &foc_val.Ualpha, &foc_val.Ubeta);
            goto open_loop;
        }
        // fSMO_MainLoop(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta);
    }

    switch (foc_mode.runmode)
    {
    case POSITION_MODE:
        if (!loop_con.fd.position_update)
            break;

        tTraj_Out traj_out = fTraj_Update(loop_con.fd.Tpos);
        foc_val.pos_ref = traj_out.value;
        foc_val.rpm_ref = fPositionRelLoopUpdate(foc_val.pos_ref, foc_val.pos_fb);
    case SPEED_MODE:
        if (!loop_con.fd.speed_update)
            break;
        if (foc_mode.runmode == SPEED_MODE)
        {
            tTraj_Out traj_out = fTraj_Update(loop_con.fd.Tspd);
            foc_val.rpm_ref = traj_out.value;
        }
        foc_val.iq_ref = fSpeedLoopUpdate(foc_val.rpm_ref, foc_val.rpm_fb);
        if (foc_val.rpm_fb > 500)
            foc_val.id_ref = fWeakMagLoopUpdate(foc_val.ud, foc_val.uq);
        else
            foc_val.id_ref = 0;

    case CURRENT_MODE:

        if (!loop_con.fd.current_update)
            break;

        foc_val.uq = fCurrentLoopUpdate(foc_val.iq_ref, foc_val.iq_fb);
        foc_val.ud = fMagLoopUpdate(foc_val.id_ref, foc_val.id_fb);
        //               foc_val.uq = foc_val.iq_ref; // 调试
        //               foc_val.ud = 0;
        fInvParkTransform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
        break;

    default: // 开环模式
        break;
    }
// 这里运行无感
open_loop:
    fSvpwmRun(foc_val.Ualpha + foc_val.Ualpha_hfi, foc_val.Ubeta + foc_val.Ubeta_hfi);
    fSamplePointCalibration();
}

// 设置各环指令值
void fFOC_SetTargetValue(float *value)
{
    switch (foc_mode.runmode)
    {
    case CURRENT_MODE:
        foc_val.iq_ref = value[0];
        foc_val.id_ref = value[1];
        break;
    case SPEED_MODE:
        fTraj_SetTarget(value[0]);
        break;
    case POSITION_MODE:
        fTraj_SetTarget(value[0]);
        if (foc_mode.pvt_mode)
            fTraj_SetRate(value[1]);
        break;
    default: // IDLE 可以直接设置ualpha和ubeta
        break;
    }
}

// 参数自动校准
bool fAutoCalibrationUpdate(void)
{
    if (TUNE_DONE == fMotorParamTune_Update(foc_val))
    {
        fFOC_CoreInit();
        return true;
    }
    return false;
}
// 设置 αβ 电压
void fFOC_SetUalphaBeta(float Ualpha, float Ubeta)
{
    foc_val.Ualpha = Ualpha;
    foc_val.Ubeta = Ubeta;
}
// 设置 dq 电流
void fFOC_SetIdIq(float id, float iq)
{
    foc_val.id_ref = id;
    foc_val.iq_ref = iq;
}
// 设置编码器零点偏移
void fSetThetaOffset(float thetaoffset)
{
    Motor.mech_offect = thetaoffset;
}

// 切换传感模式
void fFOC_SetSensorMode(eSensorMode mode)
{
    fFOC_CoreReset();
    foc_mode.sensor_mode = mode;
}
// 切换控制模式
void fFOC_SetRunMode(eRunMode mode)
{
    _FocValReset();
    foc_mode.runmode = mode;
}

// 强制刹车
bool fFOC_Shutdown(void)
{
    if (fabsf(foc_val.rpm_fb) < 0.1f)
        return true;
    if (foc_mode.runmode != SPEED_MODE)
        fFOC_SetRunMode(SPEED_MODE);
    float omega_shutdown = -foc_val.rpm_fb * 0.5f;
    fFOC_SetTargetValue(&omega_shutdown);
    return false;
}
// 设置位置零点
void fFOC_SetZeroPOS()
{
    fSetEncoderAngleZero();
}
// 设置限位位置
void fFOC_SetLimitPOS()
{
    if (foc_val.pos_fb > 0)
        g_Param.limit_position_max = foc_val.pos_fb;
    else
        g_Param.limit_position_min = foc_val.pos_fb;
    fFOC_CoreInit();
    fProSetLimitPosition(g_Param.limit_position_min, g_Param.limit_position_max);
}
