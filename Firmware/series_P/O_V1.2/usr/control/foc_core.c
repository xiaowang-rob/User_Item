#include "foc_core.h"
#include "svpwm.h"
#include "math_fast.h"
#include "device.h"
#include "string.h"
#include "smo.h"
#include "tune.h"
#include "loop_control.h"
#include "mit.h"

// HFI→SMO 融合切换速度阈值 [rpm]
#define HFI_TO_SMO_RPM  300.0f
#include "filter.h"
#include "protection_manager.h"
#include "hfi.h"
#include "usr_config.h"
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
    g_motor.mech_offset = param->theta_offset;
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
    // 从 Hz 计算 alpha = dt / (dt + 1/(2*PI*fc))
    float dt_cur = T_PWM;
    float dt_spd = T_PWM * FREQ_SPEED;

    float alpha_cur = dt_cur / (dt_cur + 1.0f / (6.2831853f * CUR_LPF_HZ));
    float alpha_spd = dt_spd / (dt_spd + 1.0f / (6.2831853f * SPEED_LPF_HZ));

    // 参数中若配置了有效 alpha 值则覆盖
    if (param->cur_filter_alpha > 0.01f && param->cur_filter_alpha < 1.0f)
        alpha_cur = param->cur_filter_alpha;
    if (param->speed_filter_alpha > 0.01f && param->speed_filter_alpha < 1.0f)
        alpha_spd = param->speed_filter_alpha;

    fFirstOrderLagInit(&_i_u_filter, alpha_cur, 0);
    fFirstOrderLagInit(&_i_v_filter, alpha_cur, 0);
    fFirstOrderLagInit(&_i_w_filter, alpha_cur, 0);
    fFirstOrderLagInit(&_omega_filter, alpha_spd, 0);
}

void filter_reset()
{
    _FilterInit(&g_Param);
}
// FOC参数更新（外部调用，参数修改后需调用）
void foc_param_update(tParameter *param)
{
    BSP_SetAdcCurrentOffset(param->adc_U_zero_offset, param->adc_V_zero_offset, param->adc_W_zero_offset);
    fEncoder_Init((eEncoderChip)param->encoder_chip);
    _MotorInit(param);
    BSP_AdcGetVoltage(&g_foc_val.udc);
    fSvpwmInit(g_foc_val.udc);
    fLoopControlInit(param, g_foc_val.udc);
    fMIT_Init(param->kp_MIT, param->kd_MIT, param->tff_MIT, param->tmax_MIT);
    _TrajectoryInit(param);

    fSmoInit(&g_motor);
    foc_set_sensor_mode(param->sensor_mode);
    g_foc_mode.run_mode = param->run_mode;
    g_foc_mode.pvt_mode = param->sw_pvt;
    foc_core_reset();
}
// FOC核心初始化
void foc_core_init(void)
{
    foc_param_update(&g_Param); // 参数加载
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
void foc_core_reset(void)
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

// 电流采样：上溢中断已通过 2-shunt 完成 Clarke，此处直接使用
static inline void _CurrentReconstruction(void)
{
    /* ialpha/ibeta 已由 BSP_SampleCurrent2Shunt 在上溢 ISR 中设置 */
    /* 仍保留一阶 LPF 去除采样噪声 */
    g_foc_val.iu = fFirstOrderLagFilter(&_i_u_filter, g_foc_val.ialpha);
    g_foc_val.iv = fFirstOrderLagFilter(&_i_v_filter, g_foc_val.ibeta);
    g_foc_val.ialpha = g_foc_val.iu;
    g_foc_val.ibeta = g_foc_val.iv;
}

void foc_value_update(void)
{
    fFrequencyDivisionUpdate();
    BSP_AdcGetVoltage(&g_foc_val.udc);
    _CurrentReconstruction();

    switch (g_foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 获取编码器数据
        g_foc_val.theta_mech = fGetEncoderAngle_ABS();
        g_foc_val.theta_elec = (g_foc_val.theta_mech - g_motor.mech_offset) * g_motor.pole_pairs * (g_motor.forward_dir ? 1 : -1) + (g_motor.elec_pi_offset ? 180 : 0);
        g_foc_val.theta_elec = fNormalizeAngle_0_360(g_foc_val.theta_elec);

        if (!g_loop_con.fd.speed_update)
            break;
        g_foc_val.pos_fb = fGetEncoderAngle_INC();
        g_foc_val.rpm_fb = fFirstOrderLagFilter(&_omega_filter, fGetEncoderRPM(F_SPEED));
        break;

    case SENSORLESS_CONTROL: {
        // 融合策略：低速用 HFI，高速用 SMO，中间线性过渡
        // HFI 和 SMO 都在后台运行，这里只做数据融合
        float smo_rpm = smo_get_omega() / g_motor.pole_pairs;
        float smo_theta = smo_get_theta();
        float hfi_theta = fHfiGetThetaElec();
        float rpm_abs = FABSF(smo_rpm);

        if (rpm_abs < HFI_TO_SMO_RPM) {
            // 低速：用 HFI
            g_foc_val.theta_elec = hfi_theta;
            g_foc_val.rpm_fb = fHfiGetOmegaElec() / g_motor.pole_pairs;
        } else if (rpm_abs < HFI_TO_SMO_RPM * 1.5f) {
            // 过渡区：线性融合
            float ratio = (rpm_abs - HFI_TO_SMO_RPM) / (HFI_TO_SMO_RPM * 0.5f);
            if (ratio > 1.0f) ratio = 1.0f;
            float hfi_omega = fHfiGetOmegaElec() / g_motor.pole_pairs;
            g_foc_val.theta_elec = hfi_theta * (1.0f - ratio) + smo_theta * ratio;
            g_foc_val.rpm_fb = hfi_omega * (1.0f - ratio) + smo_rpm * ratio;
        } else {
            // 高速：用 SMO
            g_foc_val.theta_elec = smo_theta;
            g_foc_val.rpm_fb = smo_rpm;
        }
        break;
    }
    case MERGE_CONTROL:
        g_foc_val.theta_mech = fGetEncoderAngle_ABS();
        g_foc_val.theta_elec = (g_foc_val.theta_mech - g_motor.mech_offset) * g_motor.pole_pairs * (g_motor.forward_dir ? 1 : -1) + (g_motor.elec_pi_offset ? 180 : 0);
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
// switch-case fall-through: POSITION→SPEED→CURRENT 级联控制
void foc_main_loop_task(void)
{
    if (g_foc_mode.sensor_mode >= SENSORLESS_CONTROL)
    {
        fHfiStep(g_foc_val.ialpha, g_foc_val.ibeta, &g_foc_val.ualpha_hfi, &g_foc_val.ubeta_hfi);
        // g_smo
        if (!fHfiGetStatus())
        { // todo:使能之后 直接跑电压环
            fHfiDetectInitialPosition(g_foc_val.id_fb, &g_foc_val.ualpha, &g_foc_val.ubeta);
            fSvpwmRun(g_foc_val.ualpha + g_foc_val.ualpha_hfi, g_foc_val.ubeta + g_foc_val.ubeta_hfi);
            // fSamplePointCalibration();  /* 暂不使用采样点调整 */
            return;
        }
                fSmoMainLoop(g_foc_val.ualpha, g_foc_val.ubeta, g_foc_val.ialpha, g_foc_val.ibeta);
    }
    tTraj_Out traj_out;

    switch (g_foc_mode.run_mode)
    {
    case POSITION_MODE:
        if (!g_loop_con.fd.position_update)
            break;

        traj_out = fTraj_Update(g_loop_con.fd.t_pos);
        g_foc_val.pos_ref = traj_out.value;
        g_foc_val.rpm_ref = fPositionRelLoopUpdate(g_foc_val.pos_ref, g_foc_val.pos_fb);
    case SPEED_MODE:
        if (!g_loop_con.fd.speed_update)
            break;
        if (g_foc_mode.run_mode == SPEED_MODE)
        {
            traj_out = fTraj_Update(g_loop_con.fd.t_spd);
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

    case MIT_MODE:
        if (g_loop_con.fd.speed_update)
        {
            g_foc_val.tau_ref = fMIT_LoopUpdate(g_foc_val.pos_ref, g_foc_val.pos_fb, g_foc_val.rpm_ref, g_foc_val.rpm_fb);
            g_foc_val.iq_ref = g_foc_val.tau_ref / g_motor.ke; // V·s/rad (或 V/(rad/s)) 整定出的ke要做单位换算
            g_foc_val.id_ref = 0;
        }
        if (g_loop_con.fd.current_update)
        {
            g_foc_val.uq = fCurrentLoopUpdate(g_foc_val.iq_ref, g_foc_val.iq_fb);
            g_foc_val.ud = fMagLoopUpdate(g_foc_val.id_ref, g_foc_val.id_fb);
            fInvParkTransform(g_foc_val.ud, g_foc_val.uq, sin_theta_e, cos_theta_e, &g_foc_val.ualpha, &g_foc_val.ubeta);
        }
        break;

    default: // 开环模式
        break;
    }

    fSvpwmRun(g_foc_val.ualpha + g_foc_val.ualpha_hfi, g_foc_val.ubeta + g_foc_val.ubeta_hfi);
    // fSamplePointCalibration();  /* 暂不使用采样点调整 */
}

// 设置各环指令值
void foc_set_target(float *value)
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
        foc_core_init();
        return true;
    }
    return false;
}
// 设置 αβ 电压
void foc_set_ualpha_beta(float Ualpha, float Ubeta)
{
    g_foc_val.ualpha = Ualpha;
    g_foc_val.ubeta = Ubeta;
}
// 设置 dq 电流
void foc_set_id_iq(float id, float iq)
{
    g_foc_val.id_ref = id;
    g_foc_val.iq_ref = iq;
}
// 设置编码器零点偏移
void foc_set_theta_offset(float thetaoffset)
{
    g_motor.mech_offset = thetaoffset;
}

// 切换传感模式
void foc_set_sensor_mode(eSensorMode mode)
{
    foc_core_reset();
    g_foc_mode.sensor_mode = mode;
}
// 切换控制模式
void foc_set_run_mode(eRunMode mode)
{
    _FocValReset();
    g_foc_mode.run_mode = mode;
}

// 强制刹车
bool foc_shutdown(void)
{
    if (fabsf(g_foc_val.rpm_fb) < 0.1f)
        return true;
    if (g_foc_mode.run_mode != SPEED_MODE)
        foc_set_run_mode(SPEED_MODE);
    float omega_shutdown = -g_foc_val.rpm_fb * 0.5f;
    foc_set_target(&omega_shutdown);
    return false;
}
// 设置位置零点
void foc_set_zero_pos()
{
    fSetEncoderAngleZero();
}
// 设置限位位置
void foc_set_limit_pos()
{
    if (g_foc_val.pos_fb > 0)
        g_Param.limit_position_max = g_foc_val.pos_fb;
    else
        g_Param.limit_position_min = g_foc_val.pos_fb;
    foc_core_init();
    fProSetLimitPosition(g_Param.limit_position_min, g_Param.limit_position_max);
}
