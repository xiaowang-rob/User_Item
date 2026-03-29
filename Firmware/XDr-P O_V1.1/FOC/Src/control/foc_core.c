#include "foc_core.h"
#include "adc_dr.h"
#include "svpwm.h"
#include "math_fast.h"
#include "encoder.h"
#include "string.h"
#include "smo.h"
#include "tune.h"
#include "loop_control.h"
#include "drive_parameters.h"
#include "filter.h"
#include "protection_manager.h"
#include "hfi.h"

tFOC_Mode foc_mode = {0};
tFOC_val foc_val = {0};
tMotor Motor = {0};

tFOC_Core foc_core = {.foc_mode = &foc_mode, .foc_val = &foc_val, .motor = &Motor};

/* 滤波器实例 */
static tFirstOrderLagFilter _ialpha_filter;
static tFirstOrderLagFilter _ibeta_filter;

static tFirstOrderLagFilter _omega_filter;

#define CCURRENT_FILTER_alpha 0.13f
#define SPEED_FILTER_alpha 0.04f

// 启动器初始化
static void _trajectory_init(tParameter param)
{
    tTraj_Config traj_cfg;
    traj_cfg.max_rate = param.traj_max_rate;
    traj_cfg.max_acc = param.traj_max_acc;
    traj_cfg.max_jerk = param.traj_max_jerk;
    traj_cfg.tolerance = param.tolerance;
    traj_cfg.type = param.traj_type;
    fTraj_Init(traj_cfg);
}

// 模式初始化
static void _mode_init(tParameter param)
{
    fFOC_SetSensorMode(param.sensor_mode);
    foc_mode.runmode = param.run_mode;
    foc_mode.pvt_mode = param.sw_pvt;
    foc_mode.weak_mag = param.sw_weakmag;
}

// 电机参数初始化
static void _motor_init(tParameter param)
{
    Motor.mech_offect = param.theta_offset;
    Motor.pole_pairs = param.motor_polepairs;
    Motor.elec_PI_offset = param.theta_elec_offset;
    Motor.Rs = param.motor_rs;
    Motor.Ld = param.motor_ld;
    Motor.Lq = param.motor_lq;
    Motor.Psi_f = param.motor_psif;
    Motor.Ke = param.motor_ke;
    Motor.J = param.motor_j;
    Motor.B = param.motor_b;
}

// 滤波器初始化
static void _filter_init(void)
{
    fFirstOrderLagInit(&_ialpha_filter, CCURRENT_FILTER_alpha, foc_val.Ialpha);
    fFirstOrderLagInit(&_ibeta_filter, CCURRENT_FILTER_alpha, foc_val.Ibeta);
    fFirstOrderLagInit(&_omega_filter, SPEED_FILTER_alpha, foc_val.rpm_fb);
}

// FOC核心初始化
void fFOC_CoreInit(void)
{
    fAdcGetVoltage(&Motor.Udc);
    _motor_init(g_Param);
    fLoopControlInit(g_Param, Motor.Udc / MATH_SQRT3);
    fSvpwmInit(Motor.Udc);
    _trajectory_init(g_Param);
    fFOC_CoreReset();
    _mode_init(g_Param);
			HFI_Init();
	    fSMO_Init(&Motor);	
    _filter_init();
}

// 重置FOC中间变量
static inline void _FocValReset(void)
{
    memset(&foc_val, 0, sizeof(tFOC_val));
}

// FOC 复位
void fFOC_CoreReset(void)
{
    fMotorParamTune_Reset();
    _FocValReset();
    fLoopReset();
    fSMO_Reset();
    if (foc_mode.runmode == POSITION_MODE)
        fTraj_Reset(foc_val.pos_fb);
    else if (foc_mode.runmode == SPEED_MODE)
        fTraj_Reset(foc_val.rpm_fb);
}

// 电流重构：根据扇区将线电流转换为相电流
static inline void _Current_reconstruction(void)
{
    float ui, vi, wi;
    fAdcGetCurrent(&foc_val.Iu, &foc_val.Iv, &foc_val.Iw);
    switch (fSvpwmGetSector())
    {
    case 1:
    case 6:
        ui = foc_val.Iv + foc_val.Iw;
        vi = -foc_val.Iv;
        wi = -foc_val.Iw;
        break;
    case 2:
    case 3:
        ui = -foc_val.Iu;
        vi = foc_val.Iu + foc_val.Iw;
        wi = -foc_val.Iw;
        break;
    case 4:
    case 5:
        ui = -foc_val.Iu;
        vi = -foc_val.Iv;
        wi = foc_val.Iu + foc_val.Iv;
        break;
    default:
        ui = foc_val.Iu;
        vi = -foc_val.Iv;
        wi = -foc_val.Iw;
        break;
    }
    fClarkTransform(ui, vi, wi, &foc_val.Ialpha_im, &foc_val.Ibeta_im);
    foc_val.Ialpha = fFirstOrderLagFilter(&_ialpha_filter, foc_val.Ialpha_im);
    foc_val.Ibeta = fFirstOrderLagFilter(&_ibeta_filter, foc_val.Ibeta_im);
}

void fFOC_ValueUpdate(void)
{
    fFrequencyDivisionUpdate();
    fAdcGetVoltage(&Motor.Udc);
    _Current_reconstruction();

    switch (foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 获取编码器数据
        foc_val.theta_mech = fGetEncoderAngle_ABS();
        foc_val.theta_elec = (foc_val.theta_mech - Motor.mech_offect) * Motor.pole_pairs + (Motor.elec_PI_offset ? 180 : 0);
        foc_val.theta_elec = fNormalizeAngle_0_360(foc_val.theta_elec);

        foc_val.pos_fb = fGetEncoderAngle_INC();
        foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM());
        foc_val.rpm_fb = FABSF(foc_val.rpm_fb) < 0.1 ? 0 : foc_val.rpm_fb;
        break;

    case SENSORLESS_CONTROL: // todo:运行HFI和SMO，获取其数据
//        if (0 != HFI_DetectInitialPosition(foc_val.Ialpha, foc_val.Ibeta, &foc_val.ud, &foc_val.uq))
//        { // todo:使能之后 直接跑电压环
//            fInvParkTransform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
//            break;
//        }
        // todo:这里以电角速度划分区间：低速纯HFI HFI+SMO过渡 高速SMO
        HFI_Step(foc_val.Ialpha, foc_val.Ibeta, &foc_val.Ualpha_hfi, &foc_val.Ubeta_hfi);
        // fSMO_MainLoop(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta);
        break;
    case MERGE_CONTROL:
        fSMO_MainLoop(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta);
        foc_val.theta_mech = fGetEncoderAngle_ABS();
        foc_val.theta_elec = (foc_val.theta_mech - Motor.mech_offect) * Motor.pole_pairs + (Motor.elec_PI_offset ? 180 : 0);
        foc_val.theta_elec = fNormalizeAngle_0_360(foc_val.theta_elec);

        foc_val.pos_fb = fGetEncoderAngle_INC();
        foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM());

        break;
    default:
        break;
    }
}
// 使能后执行：按模式运行对应控制环
void fFOC_MainLoopTask(void)
{

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
        if (foc_mode.weak_mag)
        {
            if (foc_val.rpm_fb > 70)
                foc_val.id_ref = fWeakMagLoopUpdate(foc_val.ud, foc_val.uq);
            else
                foc_val.id_ref = 0;
        }
    case CURRENT_MODE:
        if (!loop_con.fd.current_update)
            break;
        fParkTransform(foc_val.Ialpha, foc_val.Ibeta, foc_val.theta_elec, &foc_val.id_fb, &foc_val.iq_fb);
        // foc_val.uq = fCurrentLoopUpdate(foc_val.iq_ref, foc_val.iq_fb);
        // foc_val.ud = fMagLoopUpdate(foc_val.id_ref, foc_val.id_fb);
        foc_val.uq = foc_val.iq_ref; // 调试
        foc_val.ud = 0;
        fInvParkTransform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
        break;

    default: // 开环模式
        break;
    }

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
    if (TUNE_STATE_COMPLETE == fMotorParamTune_Update(foc_val))
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
void fSetThetaOffset(float thetaoffset, bool elec_offset)
{
    Motor.mech_offect = thetaoffset;
    Motor.elec_PI_offset = elec_offset;
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
