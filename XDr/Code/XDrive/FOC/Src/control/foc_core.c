#include "foc_core.h"
#include "adcDr.h"
#include "svpwm.h"
#include "math_fast.h"
#include "encoder.h"
#include "string.h"
#include "smo.h"
#include "loop_control.h"
#include "system_parameters.h"

FOC_mode_t foc_mode = {0};
FOC_val_t foc_val = {0};
startup_mechine_t startup_machine = {0};
Motor_t Motor = {0};

void startup_machine_init(Parameter_t param)
{
    memset(&startup_machine, 0, sizeof(startup_mechine_t));
    startup_machine.omega_gradient = param.startup_ome_grad * g_loop_con.fd.Tspd;
    startup_machine.pos_gradient = param.startup_pos_grad * g_loop_con.fd.Tpos;
    startup_machine.align_ud = param.align_current * param.motor_rs;
    startup_machine.align_steps = (u32)(param.align_time / Tcon);
    startup_machine.openloop_uq = param.open_loop_current * param.motor_rs;
    startup_machine.openloop_omega = param.open_loop_omega;
}
void mode_init(Parameter_t param)
{
    foc_mode.run_mode = param.foc_mode;
    foc_mode.loop_mode = param.loop_mode;
    foc_mode.pvt_mode = param.sw_pvt;
    foc_mode.weak_mag = param.sw_weakmag;
}
void motor_init(Parameter_t param)
{
    Motor.offset_angle = param.theta_offset;
    Motor.pole_pairs = param.motor_polepairs;
    Motor.Rs = param.motor_rs;
    Motor.Ls = param.motor_ls;
    Motor.Psi_f = param.motor_psif;
    Motor.Ke = param.motor_ke;
    Motor.J = param.motor_j;
    Motor.B = param.motor_b;
}
void auto_calibration_init(Parameter_t param)
{
    float max_omega = param.limit_omega;
    smo_init(param.motor_rs, param.motor_ls, param.motor_psif, max_omega,
             param.motor_polepairs, param.motor_ke, param.motor_j, param.motor_b);
    param_tuning_init(Motor.Udc);
}
void foc_core_init()
{
    ADC_GET_Voltage(&Motor.Udc);
    motor_init(g_Param);
    foc_core_reset();
    auto_calibration_init(g_Param);
    startup_machine_init(g_Param);
    svpwm_Init(Motor.Udc);
    mode_init(g_Param);
    loop_parameter_init(g_Param, Motor.Udc / sqrt3);
}
void startup_machine_reset()
{
    startup_machine.current_steps = 0;
}
void foc_val_reset()
{
    foc_val.iq_ref = 0;
    foc_val.id_ref = 0;
    foc_val.iq_fb = 0;
    foc_val.id_fb = 0;
    foc_val.omega_openloop = 0;
    foc_val.ud = 0;
    foc_val.uq = 0;
    foc_val.omega_ref = 0;
    foc_val.pos_ref = 0;
    foc_val.omega_con = 0;
    foc_val.pos_con = 0;
}
void foc_core_reset()
{
    memset(&foc_val, 0, sizeof(foc_val));
    loop_reset();
    smo_reset();
    startup_machine_reset();
}
// 电流重构 将采集的线电流重构为相电流
void Current_reconstruction()
{
    float ui, vi, wi;
    ADC_GET_Current(&foc_val.Iu, &foc_val.Iv, &foc_val.Iw);
    switch (svpwm_GetSector())
    {
    case 1:
        ui = foc_val.Iv + foc_val.Iw;
        vi = -foc_val.Iv;
        wi = -foc_val.Iw;
        break;
    case 2:
        ui = -foc_val.Iu;
        vi = foc_val.Iu + foc_val.Iw;
        wi = -foc_val.Iw;
        break;
    case 3:
        ui = -foc_val.Iu;
        vi = -foc_val.Iv;
        wi = foc_val.Iu + foc_val.Iv;
        break;
    case 4:
        ui = foc_val.Iv + foc_val.Iw;
        vi = -foc_val.Iv;
        wi = -foc_val.Iw;
        break;
    case 5:
        ui = -foc_val.Iu;
        vi = foc_val.Iu + foc_val.Iw;
        wi = -foc_val.Iw;
        break;
    case 6:
        ui = -foc_val.Iu;
        vi = -foc_val.Iv;
        wi = foc_val.Iu + foc_val.Iv;
    default:
        ui = foc_val.Iu;
        vi = foc_val.Iv;
        wi = foc_val.Iw;
        break;
    }
    clark_transform(ui, vi, wi, &foc_val.Ialpha, &foc_val.Ibeta);
}
// 无感 电流环初始角度对齐
bool theta_align_curloop()
{
    if (startup_machine.current_steps < startup_machine.align_steps)
    {
        startup_machine.current_steps++;
        foc_val.ud = startup_machine.align_ud;
        return false;
    }
    startup_machine.current_steps = 0;
    foc_val.ud = 0;
    foc_val.theta_elec = 0;
    startup_machine.align_flag = true;
    return true;
}
// 无感电流模式 开环切闭环
float omega_pre = 0;
void oloop_to_cloop()
{
    foc_val.uq = startup_machine.openloop_uq;
    foc_val.theta_elec += startup_machine.openloop_omega * Tcon;
    foc_val.theta_elec = normalize_angle_0_2pi(foc_val.theta_elec);
    foc_val.omega_fb = smo_get_omega();
    if (foc_val.omega_fb > startup_machine.openloop_omega * 0.7f && (foc_val.omega_fb - omega_pre) < 1)
    {
        startup_machine.change_flag = true;
    }
    omega_pre = foc_val.omega_fb;
}

// 有感foc 获取电压电流角度速度位置
void foc_encoder_get_vitop()
{
    ADC_GET_Voltage(&Motor.Udc);
    Current_reconstruction();
    foc_val.theta_mech = GET_ENCODER_ANGLE_ABS();
    foc_val.theta_elec = foc_val.theta_mech * Motor.pole_pairs;
    foc_val.theta_elec = normalize_angle_0_2pi(foc_val.theta_elec);
    park_transform(foc_val.Ialpha, foc_val.Ibeta, foc_val.theta_elec, &foc_val.id_fb, &foc_val.iq_fb);
    foc_val.omega_fb = GET_ENCODER_OMEGA();
    foc_val.pos_fb = GET_ENCODER_ANGLE_INC();
}

// 无感模式 获取电压电流角度速度位置
void foc_senless_get_vito()
{
    ADC_GET_Voltage(&Motor.Udc);
    Current_reconstruction();
    smo_update(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta);
    foc_val.omega_fb = smo_get_omega();
    if (foc_val.omega_fb > startup_machine.openloop_omega * 0.7f && (foc_val.omega_fb - omega_pre) < 1)
    {
        startup_machine.change_flag = true;
        foc_val.theta_elec = smo_get_angle();
    }
    else
    {
        if (foc_val.omega_ref > 0.1f)
        {
            foc_val.uq = startup_machine.openloop_uq;
        }
        startup_machine.change_flag = false;
        foc_val.theta_elec += startup_machine.openloop_omega * Tcon;
        foc_val.theta_elec = normalize_angle_0_2pi(foc_val.theta_elec);
    }
    omega_pre = foc_val.omega_fb;
}
void Svpwm_output()
{
    svpwm_run(foc_val.Ualpha, foc_val.Ubeta);
}
void voltage_control()
{
    inv_park_transform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
}
void current_loop_run()
{
    foc_val.uq = Current_loop(foc_val.iq_ref, foc_val.iq_fb);
    foc_val.ud = Magnetic_loop(foc_val.id_ref, foc_val.id_fb);
}
void speed_loop_run()
{
    if (!g_loop_con.fd.speed_updata)
        return;
    // 速度梯度
    if (foc_val.omega_ref - foc_val.omega_con > startup_machine.omega_gradient)
        foc_val.omega_con += startup_machine.omega_gradient;
    else if (foc_val.omega_ref - foc_val.omega_con < -startup_machine.omega_gradient)
        foc_val.omega_con -= startup_machine.omega_gradient;
    else
        foc_val.omega_con = foc_val.omega_ref;
    // 进速度环
    foc_val.iq_ref = Speed_loop(foc_val.omega_con, foc_val.omega_fb);
}
void weak_mag_loop_run()
{
    if (!g_loop_con.fd.speed_updata)
        return;
    if (foc_val.omega_fb > 30)
        foc_val.id_ref = WeakMag_loop(foc_val.ud, foc_val.uq);
    else
        foc_val.id_ref = 0;
}
void position_abs_loop_run()
{
    if (!g_loop_con.fd.position_updata)
        return;
    // 目标位置归一化
    foc_val.pos_ref = fmod(foc_val.pos_ref, M2_PI);
    // 启动过程
    if (foc_val.pos_ref - foc_val.pos_con > startup_machine.pos_gradient)
        foc_val.pos_con += startup_machine.pos_gradient;
    else if (foc_val.pos_ref - foc_val.pos_con < -startup_machine.pos_gradient)
        foc_val.pos_con -= startup_machine.pos_gradient;
    else
        foc_val.pos_con = foc_val.pos_ref;
    // 进位置环
    foc_val.omega_ref = Position_abs_loop(foc_val.pos_con, foc_val.pos_fb);
}
void position_rel_loop_run()
{
    if (!g_loop_con.fd.position_updata)
        return;
    // 启动过程
    if (foc_val.pos_ref - foc_val.pos_con > startup_machine.pos_gradient)
        foc_val.pos_con += startup_machine.pos_gradient;
    else if (foc_val.pos_ref - foc_val.pos_con < -startup_machine.pos_gradient)
        foc_val.pos_con -= startup_machine.pos_gradient;
    else
        foc_val.pos_con = foc_val.pos_ref;
    // 进位置环
    foc_val.omega_ref = Position_abs_loop(foc_val.pos_con, foc_val.pos_fb);
}
// 一直运行
void FOC_PREPARE()
{
    Frequency_division_update();
    if (foc_mode.run_mode == ENCODER_CONTROL)
    {
        foc_encoder_get_vitop();
    }
    else
    {
        foc_senless_get_vito();
    }
}

// 使能之后运行
void FOC_RUN()
{
    switch (foc_mode.run_mode)
    {
    case ENCODER_CONTROL:
        switch (foc_mode.loop_mode)
        {
        case VOLTAGE_LOOP:
            voltage_control();
            break;
        case CURRENT_LOOP:
            current_loop_run();
            voltage_control();
            break;
        case SPEED_LOOP:
            speed_loop_run();
            if (foc_mode.weak_mag)
                weak_mag_loop_run();
            current_loop_run();
            voltage_control();
            break;
        case POSITION_ABS_LOOP:
            position_abs_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        case POSITION_REL_LOOP:
            position_rel_loop_run();
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    case SENSORLESS_CONTROL:
        switch (foc_mode.loop_mode)
        {
        case VOLTAGE_LOOP:
            voltage_control();
            break;
        case CURRENT_LOOP:
            current_loop_run();
            voltage_control();
            break;
        case SPEED_LOOP:
            speed_loop_run();
            current_loop_run();
            voltage_control();
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    Svpwm_output();
    smaple_point_change();
}
void FOC_SET_OMEGA_con(float value)
{
    foc_val.omega_con = value;
}

void FOC_SET_VER_VALUE(float *value)
{
    switch (foc_mode.loop_mode)
    {
    case VOLTAGE_LOOP:
        foc_val.uq = value[0];
        foc_val.ud = value[1];
        break;
    case CURRENT_LOOP:
        foc_val.iq_ref = value[0];
        foc_val.id_ref = value[1];
    case SPEED_LOOP:
        foc_val.omega_ref = value[0];
        break;
    case POSITION_ABS_LOOP:
    case POSITION_REL_LOOP:
        foc_val.pos_ref = value[0];
        if (foc_mode.pvt_mode)
            POS_LOOP_set_omega(value[1]);
        break;
    default:
        break;
    }
}
void FOC_SET_LOOPMODE(LOOP_mode_e mode)
{
    foc_val_reset();
    foc_mode.loop_mode = mode;
}
void FOC_SET_RUNMODE(RUN_mode_e mode)
{
    foc_core_reset();
    foc_mode.run_mode = mode;
}
static u8 _tic = 0;
bool auto_calibration_update()
{
    if (_tic++ < tun_divider - 1)
        return false;
    _tic = 0;
    param_tuning_update(&foc_val.theta_elec, foc_val.theta_mech, &foc_val.Ualpha,
                        &foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta, foc_val.omega_fb,
                        Motor.pole_pairs, foc_val.iq_fb);
    if (param_tuning_get_state() == PARAM_TUNE_COMPLETE)
    {
        foc_core_init();
        return true;
    }
    return false;
}
bool SHUTDOWM()
{
    if (fabs(foc_val.omega_fb) < 0.1f)
        return true;
    if (foc_mode.loop_mode != SPEED_LOOP)
        FOC_SET_LOOPMODE(SPEED_LOOP);
    float omega_shutdown = -foc_val.omega_fb * 0.5f;
    FOC_SET_VER_VALUE(&omega_shutdown);
    return false;
}
FOC_mode_t *FOC_GET_MODE_adr()
{
    return &foc_mode;
}
FOC_val_t *FOC_GET_VAL_adr()
{
    return &foc_val;
}
startup_mechine_t *FOC_GET_STARTUP_adr()
{
    return &startup_machine;
}
Motor_t *get_motor_adr()
{
    return &Motor;
}