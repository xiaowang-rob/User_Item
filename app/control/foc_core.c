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

#include "filter.h"
#include "protection_manager.h"
#include "hfi.h"
#include "usr_config.h"
#include "bsp_adc.h"

static float sin_theta_e = 0.0f;
static float cos_theta_e = 0.0f;

tFOC_Mode foc_mode = {0};
tFOC_val foc_val = {0};
tMotor motor = {0};

tFOC_Mode *get_foc_mode_adr()
{
    return &foc_mode;
}
tFOC_val *get_foc_val_adr()
{
    return &foc_val;
}

// 滤波器实例
static tFirstOrderLagFilter _i_u_filter;
static tFirstOrderLagFilter _i_v_filter;
static tFirstOrderLagFilter _i_w_filter;

// 轨迹规划实例
static tTraj_PosOut traj_posout;
static tTraj_VelOut traj_velout;

// mit 参数
struct
{
    float acc;      // 加速度 (rad/s^2)
    float vel;      // 速度 (rad/s)
    float pos;      // 位置 (rad)
    float tau_ff;   // 转矩前馈
} mit_target = {0}; // 其中 要么加速度、速度、位置，要么位置、速度，转矩前馈（加速度和转矩前馈不同时使用）
// 转矩前馈为0,则使用加速度 转矩前馈不为0,则使用转矩前馈

// 启动器初始化
static void _trajectory_init(tParameter *param)
{
    tTraj_Config traj_cfg;
    traj_cfg.limit_d1 = param->traj_limit_d1;
    traj_cfg.limit_d2 = param->traj_limit_d2;
    traj_cfg.limit_d3 = param->traj_limit_d3;
    traj_cfg.tolerance = param->tolerance;
    traj_cfg.type = param->traj_type;
    traj_init(traj_cfg);
}

// 电机参数初始化
static void _motor_init(tParameter *param)
{
    motor.mech_offset = param->theta_offset;
    motor.pole_pairs = param->motor_polepairs;
    motor.elec_pi_offset = param->theta_elec_offset;
    motor.forward_dir = param->forward_dir;
    motor.rs = param->motor_rs;
    motor.ld = param->motor_ld;
    motor.lq = param->motor_lq;
    motor.psi_f = param->motor_psif;
    motor.ke = param->motor_ke;
    motor.j = param->motor_j;
    motor.b = param->motor_b;
}

// 滤波器初始化
static void _filter_init(tParameter *param)
{
    // 从 Hz 计算 alpha = dt / (dt + 1/(2*PI*fc))
    float dt_cur = T_PWM;

    float alpha_cur = dt_cur / (dt_cur + 1.0f / (6.2831853f * CUR_LPF_HZ));

    // 参数中若配置了有效 alpha 值则覆盖
    if (param->cur_filter_alpha > 0.01f && param->cur_filter_alpha < 1.0f)
        alpha_cur = param->cur_filter_alpha;

    filter_first_order_lag_init(&_i_u_filter, alpha_cur, 0);
    filter_first_order_lag_init(&_i_v_filter, alpha_cur, 0);
    filter_first_order_lag_init(&_i_w_filter, alpha_cur, 0);
}

// 模式初始化
static void _mode_init(tParameter *param)
{
    foc_set_sensor_mode(param->sensor_mode);
    foc_mode.run_mode = param->run_mode;
    foc_mode.pvt_mode = param->sw_pvt;
}

// 滤波器复位
void filter_reset()
{
    // 暂时没有需要复位的
}

// FOC核心初始化
void foc_core_init(tParameter *param)
{
    bsp_set_adc_current_offset(param->adc_U_zero_offset, param->adc_V_zero_offset, param->adc_W_zero_offset);
    encoder_init((eEncoderChip)param->encoder_chip);
    _motor_init(param);
    bsp_adc_get_voltage_temp(&foc_val.udc, &foc_val.temp);
    svpwm_init(foc_val.udc);
    loop_control_init(param, foc_val.udc);
    mit_init(param->kp_MIT, param->kd_MIT, motor.j, motor.b, param->tmax_MIT);
    _trajectory_init(param);

    smo_init(&motor);
    _mode_init(param);
    foc_core_reset();
    hfi_init();
    _filter_init(param);
}

// 重置FOC中间变量
static void _foc_val_reset(void)
{
    foc_val.id_ref = 0;
    foc_val.iq_ref = 0;
    foc_val.rpm_ref = 0;
    foc_val.pos_ref = 0;
    foc_val.ualpha = 0;
    foc_val.ubeta = 0;
    foc_val.ualpha_hfi = 0;
    foc_val.ubeta_hfi = 0;
    foc_val.ud = 0;
    foc_val.uq = 0;
}

// FOC 复位
void foc_core_reset(void)
{
    hfi_reset_initial_position();
    smo_reset();

    motor_param_tune_reset();

    _foc_val_reset();
    // 刷新电压，确保参数更新后电压环能正确工作
    bsp_adc_get_voltage_temp(&foc_val.udc, &foc_val.temp);
    loop_control_reset(foc_val.udc);
    svpwm_init(foc_val.udc);

    foc_val.pos_fb = encoder_get_angle_inc();
    foc_val.rpm_fb = encoder_get_rpm();
    traj_posreset(foc_val.pos_fb);
    traj_velreset(foc_val.rpm_fb);
}

// 电流重构 稳定的两相电流重构 + clark 变换
static inline void current_reconstruction(void)
{
    float ui, vi, wi;
    bsp_adc_get_current(&ui, &vi, &wi);
    u8 sector = svpwm_get_sector();
    // 根据扇区确定两相：最短导通相由另两相推导 (Ia+Ib+Ic=0)
    if (sector == 1 || sector == 6)
    { // 最短相=W
        foc_val.iu_im = vi + wi;
        foc_val.iv_im = -vi;
        foc_val.iw_im = -wi;
    }
    else if (sector == 2 || sector == 3)
    { // 最短相=U
        foc_val.iu_im = -ui;
        foc_val.iv_im = ui + wi;
        foc_val.iw_im = -wi;
    }
    else if (sector == 4 || sector == 5)
    { // 最短相=V
        foc_val.iu_im = -ui;
        foc_val.iv_im = -vi;
        foc_val.iw_im = ui + vi;
    }
    else
    { // sector 0/7: 零矢量
        foc_val.iu_im = ui;
        foc_val.iv_im = vi;
        foc_val.iw_im = wi;
    }

    foc_val.iu = filter_first_order_lag(&_i_u_filter, foc_val.iu_im);
    foc_val.iv = filter_first_order_lag(&_i_v_filter, foc_val.iv_im);
    foc_val.iw = filter_first_order_lag(&_i_w_filter, foc_val.iw_im);
    // Clarke 变换
    clarke_transform(foc_val.iu, foc_val.iv, foc_val.iw, &foc_val.ialpha, &foc_val.ibeta);
}

static bool loop_speed_update_flag = false;
static bool loop_position_update_flag = false;

// 更新foc核心变量 20khz
void foc_update_val(void)
{
    // 位置低频循环执行
    if (g_loop_con.fd.position_update)
    {
        loop_position_update_flag = true;
        bsp_adc_get_voltage_temp(&foc_val.udc, &foc_val.temp);
    }

    // 高频数据刷新
    freq_div_update();
    current_reconstruction();
    switch (foc_mode.sensor_mode)
    {
    case ENCODER_CONTROL: // 获取编码器数据
        foc_val.theta_mech = encoder_get_angle_abs();
        foc_val.theta_elec = (foc_val.theta_mech - motor.mech_offset) * motor.pole_pairs * (motor.forward_dir ? 1 : -1) + (motor.elec_pi_offset ? 180 : 0);
        foc_val.theta_elec = normalize_angle_360(foc_val.theta_elec);

        //  这样可以保证速度环计算和位置环计算 与 中频低频值计算 不在同一周期执行 且数据在前一周期完成计算
        if (g_loop_con.fd.speed_update)
        {
            loop_speed_update_flag = true;
            encoder_pll_update(g_loop_con.fd.t_spd);
            foc_val.pos_fb = encoder_get_angle_inc();
            foc_val.rpm_fb = encoder_get_rpm();
        }

        break;

    case SENSORLESS_CONTROL:
    {
        // 融合策略：低速用 HFI，高速用 SMO，中间线性过渡
        // HFI 和 SMO 都在后台运行，这里只做数据融合
        float smo_rpm = smo_get_omega() / motor.pole_pairs;
        float smo_theta = smo_get_theta();
        float hfi_theta = hfi_get_theta_elec();
        float rpm_abs = FABSF(smo_rpm);

        // if (rpm_abs < HFI_TO_SMO_RPM)
        // {
        //     // 低速：用 HFI
        //     foc_val.theta_elec = hfi_theta;
        //     foc_val.rpm_fb = hfi_get_omega_elec() / motor.pole_pairs;
        // }
        // else if (rpm_abs < HFI_TO_SMO_RPM * 1.5f)
        // {
        //     // 过渡区：线性融合
        //     float ratio = (rpm_abs - HFI_TO_SMO_RPM) / (HFI_TO_SMO_RPM * 0.5f);
        //     if (ratio > 1.0f)
        //         ratio = 1.0f;
        //     float hfi_omega = hfi_get_omega_elec() / motor.pole_pairs;
        //     foc_val.theta_elec = hfi_theta * (1.0f - ratio) + smo_theta * ratio;
        //     foc_val.rpm_fb = hfi_omega * (1.0f - ratio) + smo_rpm * ratio;
        // }
        // else
        // {
        //     // 高速：用 SMO
        //     foc_val.theta_elec = smo_theta;
        //     foc_val.rpm_fb = smo_rpm;
        // }
        break;
    }
    case MERGE_CONTROL:
        foc_val.theta_mech = encoder_get_angle_abs();
        foc_val.theta_elec = (foc_val.theta_mech - motor.mech_offset) * motor.pole_pairs * (motor.forward_dir ? 1 : -1) + (motor.elec_pi_offset ? 180 : 0);
        foc_val.theta_elec = normalize_angle_360(foc_val.theta_elec);

        if (g_loop_con.fd.speed_update)
        {
            loop_speed_update_flag = true;
            encoder_pll_update(g_loop_con.fd.t_spd);
            foc_val.pos_fb = encoder_get_angle_inc();
            foc_val.rpm_fb = encoder_get_rpm();
        }
        break;
    default:
        break;
    }
    arm_sin_cos_f32(foc_val.theta_elec, &sin_theta_e, &cos_theta_e);
    park_transform(foc_val.ialpha, foc_val.ibeta, sin_theta_e, cos_theta_e, &foc_val.id_fb, &foc_val.iq_fb);
}

// 使能后执行：按模式运行对应控制环
void foc_main_loop_task(void)
{
    if (foc_mode.sensor_mode >= SENSORLESS_CONTROL) // 无感和融合控制
    {
        // HFI 无感观测器
        hfi_step(foc_val.ialpha, foc_val.ibeta, &foc_val.ualpha_hfi, &foc_val.ubeta_hfi);

        if (!hfi_get_status()) // 初始极性辨识
        {
            // todo:使能之后 直接跑电压环
            hfi_detect_initial_position(foc_val.id_fb, &foc_val.ualpha, &foc_val.ubeta);
            svpwm_run(foc_val.ualpha + foc_val.ualpha_hfi, foc_val.ubeta + foc_val.ubeta_hfi);
            // svpwm_sample_point_calibration();  // 暂不使用采样点调整
            return;
        }
        // SMO 无感观测器
        // smo_main_loop(foc_val.ualpha, foc_val.ubeta, foc_val.ialpha, foc_val.ibeta);
    }

    switch (foc_mode.run_mode)
    {

    case PID_POSITION:
        if (g_loop_con.fd.position_update) // 前一周期计算
        {
            traj_posout = traj_PosUpdate(g_loop_con.fd.t_pos);
            foc_val.pos_ref = traj_posout.value;
        }
        if (loop_position_update_flag)
        {
            loop_position_update_flag = false;
            foc_val.rpm_ref = loop_position_update(foc_val.pos_ref, foc_val.pos_fb);
        }
        if (loop_speed_update_flag)
        {
            loop_speed_update_flag = false;
            foc_val.iq_ref = loop_speed_update(foc_val.rpm_ref, foc_val.rpm_fb);
            foc_val.id_ref = loop_weak_mag_update(foc_val.ud, foc_val.uq);
        }
        goto current_loop;

    case PID_SPEED:
        if (g_loop_con.fd.speed_update) // 前一周期计算
        {
            traj_velout = traj_VelUpdate(g_loop_con.fd.t_spd);
            foc_val.rpm_ref = traj_velout.value;
        }
        if (loop_speed_update_flag)
        {
            loop_speed_update_flag = false;
            foc_val.iq_ref = loop_speed_update(foc_val.rpm_ref, foc_val.rpm_fb);
            foc_val.id_ref = loop_weak_mag_update(foc_val.ud, foc_val.uq);
        }
        goto current_loop;

    case MIT_POSITION:                  // TODO: 要做单位换算
        if (g_loop_con.fd.speed_update) // 前一周期计算
        {
            traj_posout = traj_PosUpdate(g_loop_con.fd.t_spd);
            foc_val.pos_ref = traj_posout.value;
        }
        if (loop_speed_update_flag)
        {
            loop_speed_update_flag = false;
            foc_val.tau_ref = mit_pos_update(traj_posout.accel, foc_val.pos_ref, foc_val.pos_fb, foc_val.rpm_fb);
            foc_val.iq_ref = foc_val.tau_ref / motor.ke; // V·s/rad (或 V/(rad/s)) 整定出的ke要做单位换算
            foc_val.id_ref = 0;
        }
        goto current_loop;

    case MIT_SPEED:
        if (g_loop_con.fd.speed_update) // 前一周期计算
        {
            traj_velout = traj_VelUpdate(g_loop_con.fd.t_spd);
            foc_val.rpm_ref = traj_velout.value;
        }
        if (loop_speed_update_flag)
        {
            loop_speed_update_flag = false;
            foc_val.tau_ref = mit_vel_update(traj_velout.accel, foc_val.rpm_ref, foc_val.rpm_fb);
            foc_val.iq_ref = foc_val.tau_ref / motor.ke; // V·s/rad (或 V/(rad/s)) 整定出的ke要做单位换算
            foc_val.id_ref = 0;
        }
        goto current_loop;

    case MIT_TRAJ: // 不能用轨迹规划
        if (loop_speed_update_flag)
        {
            foc_val.tau_ref = mit_track_update(traj_posout.accel, foc_val.tau_ff_ref, foc_val.pos_ref, foc_val.pos_fb, foc_val.rpm_ref, foc_val.rpm_fb);
            foc_val.iq_ref = foc_val.tau_ref / motor.ke; // V·s/rad (或 V/(rad/s)) 整定出的ke要做单位换算
            foc_val.id_ref = 0;
        }
        goto current_loop;

        // 电流环
    case CURRENT_MODE:
    current_loop:
        foc_val.uq = loop_current_update(foc_val.iq_ref, foc_val.iq_fb);
        foc_val.ud = loop_mag_update(foc_val.id_ref, foc_val.id_fb);
        inv_park_transform(foc_val.ud, foc_val.uq, sin_theta_e, cos_theta_e, &foc_val.ualpha, &foc_val.ubeta);
        break;
        // 开环测试
    case OPEN_LOOP:

        break;
    default:
        break;
    }

    svpwm_run(foc_val.ualpha + foc_val.ualpha_hfi, foc_val.ubeta + foc_val.ubeta_hfi);
    // svpwm_sample_point_calibration();  // 暂不使用采样点调整
}

// 设置各环指令值
void foc_set_target(float *value)
{
    switch (foc_mode.run_mode)
    {
    case CURRENT_MODE:
        foc_val.iq_ref = value[0];
        foc_val.id_ref = value[1];
        break;
    case PID_SPEED:
        traj_set_veltarget(value[0]);
        break;
    case PID_POSITION:
        if (foc_mode.pvt_mode == PVT_PV)
        { // TODO: 实现 PV闭环控制 不能与 轨迹规划同时使用
            float max_rpm = FABSF(value[1]);

            if (max_rpm <= 0.01f)
                max_rpm = 0.01f;
            if (max_rpm > g_Param.limit_omega)
                max_rpm = g_Param.limit_omega;
            loop_set_position_out_limit(max_rpm);
        }
        else if (foc_mode.pvt_mode == PVT_PT)
        { // TODO: 实现 PT闭环控制
        }
        else
            traj_set_postarget(value[0]);
        break;
    case MIT_POSITION:
        traj_set_postarget(value[0]);
        break;
    case MIT_SPEED:
        traj_set_veltarget(value[0]);
        break;
    case MIT_TRAJ:
        // TODO:  到时候四个can帧发送完成后同时写入 目标位置 目标速度 目标加速度 前馈扭矩
        // 不能用内置轨迹规划器
        mit_target.acc = value[0];
        mit_target.vel = value[1];
        mit_target.pos = value[2];
        mit_target.tau_ff = value[3];
        break;
    default: // IDLE 可以直接设置ualpha和ubeta
        break;
    }
}

// 设置 αβ 电压
void foc_set_ualpha_beta(float Ualpha, float Ubeta)
{
    foc_val.ualpha = Ualpha;
    foc_val.ubeta = Ubeta;
}
// 设置 dq 电流
void foc_set_id_iq(float id, float iq)
{
    foc_val.id_ref = id;
    foc_val.iq_ref = iq;
}
// 设置编码器零点偏移
void foc_set_theta_offset(float thetaoffset)
{
    motor.mech_offset = thetaoffset;
}

// 切换传感模式
void foc_set_sensor_mode(eSensorMode mode)
{
    foc_core_reset();
    foc_mode.sensor_mode = mode;
}
// 切换控制模式
void foc_set_run_mode(eRunMode mode)
{
    _foc_val_reset();
    foc_mode.run_mode = mode;
}

// 强制刹车
bool foc_shutdown(void)
{
    if (fabsf(foc_val.rpm_fb) < 0.1f)
        return true;
    foc_set_run_mode(PID_SPEED);
    float omega_shutdown = -foc_val.rpm_fb * 0.5f;
    foc_set_target(&omega_shutdown);
    return false;
}
// 设置位置零点
void foc_set_zero_pos()
{
    encoder_set_angle_zero();
}
// 设置限位位置
void foc_set_limit_pos()
{
    if (foc_val.pos_fb > 0)
        g_Param.limit_position_max = foc_val.pos_fb;
    else
        g_Param.limit_position_min = foc_val.pos_fb;
    foc_core_init(&g_Param);
    pro_set_limit_position(g_Param.limit_position_min, g_Param.limit_position_max);
}
