#include "foc_core.h"
#include "adcDr.h"
#include "svpwm.h"
#include "auto_calibration.h"
#include "math_fast.h"
#include "encoder.h"
#include "string.h"
#include "smo.h"
#include "loop_control.h"
#include "system_parameters.h"
#include "stream_transmission.h"
#include "adaptive_control.h"

// smo_sensorless_t smo = {0};
// SVPWM_t g_svpwm = {.sector = 1};
FOC_val_t foc_val = {0};
startup_mechine_t startup_machine = {0};
void startup_machine_init(LOOP_CON_t *loopcon, float omega_gradient, float pos_gradient,
                          float align_voltage, float align_time, float openloop_voltage,
                          float openloop_omega)
{
    memset(&startup_machine, 0, sizeof(startup_mechine_t));
    startup_machine.omega_gradient = omega_gradient * loopcon->fd.Tspd;
    startup_machine.pos_gradient = pos_gradient * loopcon->fd.Tpos;
    startup_machine.align_ud = align_voltage;
    startup_machine.align_steps = (u32)(align_time / Tcon);
    startup_machine.openloop_uq = openloop_voltage;
    startup_machine.openloop_omega = openloop_omega;
}
void startup_machine_reset()
{
    startup_machine.current_steps = 0;
    startup_machine.change_flag = false;
}
bool foc_core_init()
{
    Frequency_division_init(g_foc_parameters.f_speed_loop, g_foc_parameters.f_position_loop);
    PI_init(&g_loop_con.PI_id, g_foc_parameters.kp_current, g_foc_parameters.ki_current, g_adaptive_con.Udc);
    PI_init(&g_loop_con.PI_iq, g_foc_parameters.kp_current, g_foc_parameters.ki_current, g_adaptive_con.Udc);
    PI_init(&g_loop_con.PI_weakmag, g_foc_parameters.kp_weakmag, g_foc_parameters.ki_weakmag, g_foc_parameters.limit_current);
    PI_init(&g_loop_con.PI_speed, g_foc_parameters.kp_speed, g_foc_parameters.ki_speed, g_foc_parameters.limit_current);
    PID_init(&g_loop_con.PID_pos, g_foc_parameters.kp_position, g_foc_parameters.ki_position, g_foc_parameters.kd_position, g_foc_parameters.limit_speed);
    g_loop_con.position_min = g_foc_parameters.limit_position_min;
    g_loop_con.position_max = g_foc_parameters.limit_position_max;
    smo_sensorless_init(&smo, g_foc_parameters.motor_rs, g_foc_parameters.motor_ls, Tcon);
    auto_calibration_init(g_foc_parameters.motor_rs, g_foc_parameters.motor_ls, g_foc_parameters.motor_psif, g_foc_parameters.motor_polepairs, (TUNE_MODE_E)g_foc_mode.foc_autotune_mode);
    startup_machine_init(&g_loop_con, g_foc_parameters.startup_spe_grad, g_foc_parameters.startup_pos_grad, g_foc_parameters.align_current, g_foc_parameters.align_time, g_foc_parameters.open_loop_current, g_foc_parameters.open_loop_speed, g_foc_parameters.change_loop_speed);
    svpwm_Init(g_svpwm, g_adaptive_con.Udc);
    foc_val.run_mode = (RUN_MODE_e)g_foc_mode.foc_run_mode;
    foc_val.loop_mode = (LOOP_MODE_e)g_foc_mode.foc_loop_mode;
    if (foc_val.run_mode == ENCODER_CONTROL)
        ENCODER_Init();
    return true;
}
void foc_core_reset()
{
    Frequency_division_reset();
    loop_reset();
    smo_sensorless_reset(&smo);
    startup_machine_reset();
    svpwm_SetVbus(g_svpwm, g_adaptive_con.Udc);
}
// 电流重构 将采集的线电流重构为相电流
void Current_reconstruction(SVPWM_t *svpwm)
{
    float ui, vi, wi;
    ADC_GET_Current(&foc_val.Iu, &foc_val.Iv, &foc_val.Iw);
    switch (svpwm->sector)
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
        ui = 0;
        vi = 0;
        wi = 0;
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
    foc_val.omega_fb = smo_sensorless_get_omega(&smo);
    if (foc_val.omega_fb > startup_machine.openloop_omega * 0.7f && (foc_val.omega_fb - omega_pre) < 1)
    {
        startup_machine.change_flag = true;
    }
    omega_pre = foc_val.omega_fb;
}
// 电流采样点改变
u8 change_Index = 0;
void smaple_point_change(SVPWM_t *svpwm)
{
    switch (svpwm->sector)
    {
    case 1:
    case 4:
        if (svpwm->ticu > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * svpwm->ticu)
        {
            if (change_Index != 3)
                change_Index = 3;
            else
                return;
        }
        else
        {
            if (change_Index != 2)
                change_Index = 2;
            else
                return;
        }
        break;
    case 2:
    case 5:
        if (svpwm->ticv > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * svpwm->ticv)
        {
            if (change_Index != 3)
                change_Index = 3;
            else
                return;
        }
        else
        {
            if (change_Index != 2)
                change_Index = 2;
            else
                return;
        }
        break;
    default:
        if (svpwm->ticw > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * svpwm->ticw)
        {
            if (change_Index != 3)
                change_Index = 3;
            else
                return;
        }
        else
        {
            if (change_Index != 2)
                change_Index = 2;
            else
                return;
        }
        break;
    }

    switch (change_Index)
    {
    case 1:
        ADC_sample_change(1);
        break;
    case 2:
        ADC_sample_change(svpwm->ticu - tics);
        break;
    case 3:
        ADC_sample_change(svpwm->ticu + ticDT + ticTN);
        break;
    default:
        break;
    }
}
// 有感foc
void foc_encoder_prepare()
{
    Frequency_division_update();
    Current_reconstruction(&g_svpwm);
    // todo:
    foc_val.theta_mech = GET_ENCODER_ANGLE_ABS();
    foc_val.theta_elec = foc_val.theta_mech * g_Motor.pole_pairs;
    foc_val.theta_elec = normalize_angle_0_2pi(foc_val.theta_elec);
    park_transform(foc_val.Ialpha, foc_val.Ibeta, foc_val.theta_elec, &foc_val.id_fb, &foc_val.iq_fb);
    foc_val.omega_fb = GET_ENCODER_OMEGA();
    foc_val.pos_fb = GET_ENCODER_ANGLE_INC();
}
void foc_senless_prepare()
{
    Frequency_division_update();
    Current_reconstruction(&g_svpwm);
    // todo:
}
void position_loop_run(f_Division_t fd)
{
    if (fd.position_updata)
}
void foc_core_run()
{
    Frequency_division_update();
    Current_reconstruction(&g_svpwm);
    switch (foc_val.run_mode)
    {
    case ENCODER_CONTROL:

        switch (foc_val.loop_mode)
        {
        case POSITION_ABS_CONTROL:
            foc_val.pos_fb = GET_ENCODER_ANGLE_ABS();
            if (g_loop_con.fd.position_updata)
            {
                // 目标位置归一化
                if (foc_val.pos_ref > M2_PI)
                    foc_val.pos_ref -= M2_PI;
                if (foc_val.pos_ref < -M2_PI)
                    foc_val.pos_ref += M2_PI;
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
            // 进速度环
            if (g_loop_con.fd.speed_updata)
            {
                if (foc_val.omega_ref - foc_val.omega_con > startup_machine.omega_gradient)
                    foc_val.omega_con += startup_machine.omega_gradient;
                if (foc_val.omega_ref - foc_val.omega_con < -startup_machine.omega_gradient)
                    foc_val.omega_con -= startup_machine.omega_gradient;
                foc_val.iq_ref = Speed_loop(foc_val.omega_con, foc_val.omega_fb);
            }
            break;
        case POSITION_REL_CONTROL:
            foc_val.pos_fb = GET_ENCODER_ANGLE_INC();
            if (g_loop_con.fd.position_updata)
            {
                if (foc_val.pos_ref - foc_val.pos_con > startup_machine.pos_gradient)
                    foc_val.pos_con += startup_machine.pos_gradient;
                else if (foc_val.pos_ref - foc_val.pos_con < -startup_machine.pos_gradient)
                    foc_val.pos_con -= startup_machine.pos_gradient;
                else
                    foc_val.pos_con = foc_val.pos_ref;
                foc_val.omega_ref = Position_rel_loop(foc_val.pos_con, foc_val.pos_fb);
            }
            if (g_loop_con.fd.speed_updata)
            {
                if (foc_val.omega_ref - foc_val.omega_con > startup_machine.omega_gradient)
                    foc_val.omega_con += startup_machine.omega_gradient;
                if (foc_val.omega_ref - foc_val.omega_con < -startup_machine.omega_gradient)
                    foc_val.omega_con -= startup_machine.omega_gradient;
                foc_val.iq_ref = Speed_loop(foc_val.omega_con, foc_val.omega_fb);
            }
            break;
        case SPEED_LOOP_CONTROL:
            if (g_loop_con.fd.speed_updata)
            {
                if (foc_val.omega_ref - foc_val.omega_con > startup_machine.omega_gradient)
                    foc_val.omega_con += startup_machine.omega_gradient;
                if (foc_val.omega_ref - foc_val.omega_con < -startup_machine.omega_gradient)
                    foc_val.omega_con -= startup_machine.omega_gradient;
                foc_val.iq_ref = Speed_loop(foc_val.omega_con, foc_val.omega_fb);
                if (g_adaptive_con.weakmag_enable) // 弱磁模式
                    if (foc_val.omega_fb > 30)
                        foc_val.id_ref = WeakMag_loop(foc_val.ud, foc_val.uq, g_adaptive_con.max_Vs);
            }
            break;
        default:
            break;
        }
        foc_val.uq = Current_loop(foc_val.iq_ref, foc_val.iq_fb);
        foc_val.ud = Magnetic_loop(foc_val.id_ref, foc_val.id_fb);
        if (foc_val.uq == g_loop_con.PI_iq.output_limit)
            inv_park_transform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
        break;
    case SENSORLESS_CONTROL:
        if (!startup_machine.align_flag)
            theta_align();
        else
        {
            smo_sensorless_update(&smo, foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta);
            if (!startup_machine.change_flag)
                oloop_to_cloop();
            else
            {
                foc_val.theta_elec = smo.theta;
                if (foc_val.omega_ref > foc_val.omega_con && foc_val.omega_fb < foc_val.omega_con)
                    startup_machine.change_flag = false;
            }
            park_transform(foc_val.Ialpha, foc_val.Ibeta, foc_val.theta_elec, &foc_val.id_fb, &foc_val.iq_fb);
            foc_val.omega_fb = smo_sensorless_get_omega(&smo);
            if (foc_val.loop_mode == SPEED_LOOP_CONTROL)
            {
                if (g_loop_con.fd.speed_updata)
                {
                    if (foc_val.omega_ref - foc_val.omega_con > startup_machine.omega_gradient)
                        foc_val.omega_con += startup_machine.omega_gradient;
                    if (foc_val.omega_ref - foc_val.omega_con < -startup_machine.omega_gradient)
                        foc_val.omega_con -= startup_machine.omega_gradient;
                    foc_val.iq_ref = Speed_loop(foc_val.omega_con, foc_val.omega_fb);
                }
            }
        }
        foc_val.uq = Current_loop(foc_val.iq_ref, foc_val.iq_fb);
        foc_val.ud = Magnetic_loop(foc_val.id_ref, foc_val.id_fb);
        if (foc_val.uq == g_loop_con.PI_iq.output_limit)
            inv_park_transform(foc_val.ud, foc_val.uq, foc_val.theta_elec, &foc_val.Ualpha, &foc_val.Ubeta);
        break;
    default:
        break;
    }
    svpwm_run(foc_val.Ualpha, foc_val.Ubeta, g_svpwm);
    smaple_point_change();
}

void CONTROL_value_update(float *data)
{
    switch (foc_val.loop_mode)
    {
    case CURRENT_LOOP_CONTROL:
        foc_val.iq_ref = data[0];
        break;
    case SPEED_LOOP_CONTROL:
        foc_val.omega_ref = rpm_to_rad(data[0]);
        break;
    case POSITION_ABS_CONTROL:
    case POSITION_REL_CONTROL:
        if (foc_val.pvt_mode)
        {
            foc_val.pos_ref = deg_to_rad(data[0]);
            g_loop_con.PID_pos.output_limit = deg_to_rad(data[1]);
        }
        else
            foc_val.pos_ref = deg_to_rad(data[0]);
        break;
    default:
        break;
    }
}
void CONTROL_mode_updata(u8 mode)
{
    foc_val.loop_mode = mode;
}
