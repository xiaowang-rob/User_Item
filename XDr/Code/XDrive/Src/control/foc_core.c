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

Monitor_t g_monitor = {0};
foc_core_t g_foccore = {0};
smo_sensorless_t smo = {0};
SVPWM_t g_svpwm = {0};
startup_mechine_t startup_machine;
void startup_machine_init(LOOP_CON_t *loopcon, float omega_gradient, float pos_gradient, float align_current, float align_time, float openloop_cur, float openloop_speed, float changeloop_speed)
{
    memset(&startup_machine, 0, sizeof(startup_mechine_t));
    startup_machine.omega_gradient = omega_gradient * loopcon->fd.Tspd;
    startup_machine.pos_gradient = pos_gradient * loopcon->fd.Tpos;
    startup_machine.align_cur = align_current;
    startup_machine.align_steps = (u32)(align_time / Tcon);
    startup_machine.openloop_cur = openloop_cur;
    startup_machine.openloop_omega = openloop_speed;
    startup_machine.changeloop_speed = changeloop_speed;
}
void startup_machine_reset()
{
    startup_machine.current_steps = 0;
    startup_machine.align_flag = false;
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
    g_foccore.run_mode = (RUN_MODE_e)g_foc_mode.foc_run_mode;
    g_foccore.loop_mode = (LOOP_MODE_e)g_foc_mode.foc_loop_mode;
    if (g_foccore.run_mode == ENCODER_CONTROL)
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
void Current_reconstruction()
{
    float ui, vi, wi;
    ADC_GET_Current(&g_monitor.Iu, &g_monitor.Iv, &g_monitor.Iw);
    switch (g_svpwm.sector)
    {
    case 1:
        ui = g_monitor.Iv + g_monitor.Iw;
        vi = -g_monitor.Iv;
        wi = -g_monitor.Iw;
        break;
    case 2:
        ui = -g_monitor.Iu;
        vi = g_monitor.Iu + g_monitor.Iw;
        wi = -g_monitor.Iw;
        break;
    case 3:
        ui = -g_monitor.Iu;
        vi = -g_monitor.Iv;
        wi = g_monitor.Iu + g_monitor.Iv;
        break;
    case 4:
        ui = g_monitor.Iv + g_monitor.Iw;
        vi = -g_monitor.Iv;
        wi = -g_monitor.Iw;
        break;
    case 5:
        ui = -g_monitor.Iu;
        vi = g_monitor.Iu + g_monitor.Iw;
        wi = -g_monitor.Iw;
        break;
    case 6:
        ui = -g_monitor.Iu;
        vi = -g_monitor.Iv;
        wi = g_monitor.Iu + g_monitor.Iv;
    default:
        ui = 0;
        vi = 0;
        wi = 0;
        break;
    }
    clark_transform(ui, vi, wi, &g_monitor.Ialpha, &g_monitor.Ibeta);
}
void theta_align()
{
    if (startup_machine.current_steps < startup_machine.align_steps)
    {
        startup_machine.current_steps++;
        g_foccore.id_ref = startup_machine.align_cur;
    }
    else
    {
        startup_machine.current_steps = 0;
        g_foccore.id_ref = 0;
        g_monitor.theta_elec = 0;
        startup_machine.align_flag = true;
    }
}
float openloop_omega_con = 2.0f; // 约20转
void oloop_to_cloop()            // 无感模式 开环切闭环
{
    if (openloop_omega_con < startup_machine.openloop_omega)
        openloop_omega_con += 0.001f;
    else
        openloop_omega_con = startup_machine.openloop_omega;
    g_monitor.theta_elec += openloop_omega_con * Tcon;
    if (g_monitor.theta_elec > M2_PI)
        g_monitor.theta_elec -= M2_PI;
    g_foccore.iq_ref = startup_machine.openloop_cur;
    g_monitor.omega_fb = smo_sensorless_get_omega(&smo);
    if (g_monitor.omega_fb > startup_machine.changeloop_speed && openloop_omega_con > startup_machine.changeloop_speed * 0.7f)
    {
        startup_machine.change_flag = true;
        openloop_omega_con = 2.0f;
    }
}

u8 change_Index = 0;
void smaple_point_change()
{
    switch (g_svpwm.sector)
    {
    case 1:
    case 4:
        if (g_svpwm.ticu > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * g_svpwm.ticu)
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
        if (g_svpwm.ticv > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * g_svpwm.ticv)
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
        if (g_svpwm.ticw > ticDT + ticTN)
        {
            if (change_Index != 1)
                change_Index = 1;
            else
                return;
        }
        else if (alltic_tsdttn > 2 * g_svpwm.ticw)
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
        ADC_sample_change(g_svpwm.ticu - tics);
        break;
    case 3:
        ADC_sample_change(g_svpwm.ticu + ticDT + ticTN);
        break;
    default:
        break;
    }
}

void foc_core_run()
{
    Frequency_division_updatta();
    Current_reconstruction();
    switch (g_foccore.run_mode)
    {
    case ENCODER_CONTROL:
        g_monitor.theta_elec = GET_ENCODER_ANGLE_ABS() * g_Motor.pole_pairs;
        park_transform(g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_elec, &g_monitor.id_fb, &g_monitor.iq_fb);
        g_monitor.theta_mech = GET_ENCODER_ANGLE_ABS();
        g_monitor.omega_fb = GET_ENCODER_OMEGA();
        switch (g_foccore.loop_mode)
        {
        case POSITION_ABS_CONTROL:
            g_monitor.pos_fb = GET_ENCODER_ANGLE_ABS();
            if (g_loop_con.fd.position_updata)
            {
                // 目标位置归一化
                if (g_foccore.pos_ref > M2_PI)
                    g_foccore.pos_ref -= M2_PI;
                if (g_foccore.pos_ref < -M2_PI)
                    g_foccore.pos_ref += M2_PI;
                // 启动过程
                if (g_foccore.pos_ref - g_foccore.pos_con > startup_machine.pos_gradient)
                    g_foccore.pos_con += startup_machine.pos_gradient;
                else if (g_foccore.pos_ref - g_foccore.pos_con < -startup_machine.pos_gradient)
                    g_foccore.pos_con -= startup_machine.pos_gradient;
                else
                    g_foccore.pos_con = g_foccore.pos_ref;
                // 进位置环
                g_foccore.omega_ref = Position_abs_loop(g_foccore.pos_con, g_monitor.pos_fb);
            }
            // 进速度环
            if (g_loop_con.fd.speed_updata)
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
            break;
        case POSITION_REL_CONTROL:
            g_monitor.pos_fb = GET_ENCODER_ANGLE_INC();
            if (g_loop_con.fd.position_updata)
            {
                if (g_foccore.pos_ref - g_foccore.pos_con > startup_machine.pos_gradient)
                    g_foccore.pos_con += startup_machine.pos_gradient;
                else if (g_foccore.pos_ref - g_foccore.pos_con < -startup_machine.pos_gradient)
                    g_foccore.pos_con -= startup_machine.pos_gradient;
                else
                    g_foccore.pos_con = g_foccore.pos_ref;
                g_foccore.omega_ref = Position_rel_loop(g_foccore.pos_con, g_monitor.pos_fb);
            }
            if (g_loop_con.fd.speed_updata)
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
            }
            break;
        case SPEED_LOOP_CONTROL:
            if (g_loop_con.fd.speed_updata)
            {
                if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                    g_foccore.omega_con += startup_machine.omega_gradient;
                if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                    g_foccore.omega_con -= startup_machine.omega_gradient;
                g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
                if (g_adaptive_con.weakmag_enable) // 弱磁模式
                    if (g_monitor.omega_fb > 30)
                        g_foccore.id_ref = WeakMag_loop(g_monitor.ud, g_monitor.uq, g_adaptive_con.max_Vs);
            }
            break;
        default:
            break;
        }
        g_monitor.uq = Current_loop(g_foccore.iq_ref, g_monitor.iq_fb);
        g_monitor.ud = Magnetic_loop(g_foccore.id_ref, g_monitor.id_fb);
        if (g_monitor.uq == g_loop_con.PI_iq.output_limit)
            inv_park_transform(g_monitor.ud, g_monitor.uq, g_monitor.theta_elec, &g_monitor.Ualpha, &g_monitor.Ubeta);
        break;
    case SENSORLESS_CONTROL:
        if (!startup_machine.align_flag)
            theta_align();
        else
        {
            smo_sensorless_update(&smo, g_monitor.Ualpha, g_monitor.Ubeta, g_monitor.Ialpha, g_monitor.Ibeta);
            if (!startup_machine.change_flag)
                oloop_to_cloop();
            else
            {
                g_monitor.theta_elec = smo.theta;
                if (g_foccore.omega_ref > openloop_omega_con && g_monitor.omega_fb < openloop_omega_con)
                    startup_machine.change_flag = false;
            }
            park_transform(g_monitor.Ialpha, g_monitor.Ibeta, g_monitor.theta_elec, &g_monitor.id_fb, &g_monitor.iq_fb);
            g_monitor.omega_fb = smo_sensorless_get_omega(&smo);
            if (g_foccore.loop_mode == SPEED_LOOP_CONTROL)
            {
                if (g_loop_con.fd.speed_updata)
                {
                    if (g_foccore.omega_ref - g_foccore.omega_con > startup_machine.omega_gradient)
                        g_foccore.omega_con += startup_machine.omega_gradient;
                    if (g_foccore.omega_ref - g_foccore.omega_con < -startup_machine.omega_gradient)
                        g_foccore.omega_con -= startup_machine.omega_gradient;
                    g_foccore.iq_ref = Speed_loop(g_foccore.omega_con, g_monitor.omega_fb);
                }
            }
        }
        g_monitor.uq = Current_loop(g_foccore.iq_ref, g_monitor.iq_fb);
        g_monitor.ud = Magnetic_loop(g_foccore.id_ref, g_monitor.id_fb);
        if (g_monitor.uq == g_loop_con.PI_iq.output_limit)
            inv_park_transform(g_monitor.ud, g_monitor.uq, g_monitor.theta_elec, &g_monitor.Ualpha, &g_monitor.Ubeta);
        break;
    default:
        break;
    }
    svpwm_run(g_monitor.Ualpha, g_monitor.Ubeta, g_svpwm);
    smaple_point_change();
//		ADC1_sample();
}

void CONTROL_value_update(float *data)
{
    switch (g_foccore.loop_mode)
    {
    case CURRENT_LOOP_CONTROL:
        g_foccore.iq_ref = data[0];
        break;
    case SPEED_LOOP_CONTROL:
        g_foccore.omega_ref = rpm_to_rad(data[0]);
        break;
    case POSITION_ABS_CONTROL:
    case POSITION_REL_CONTROL:
        if (g_foccore.pvt_mode)
        {
            g_foccore.pos_ref = deg_to_rad(data[0]);
            g_loop_con.PID_pos.output_limit = deg_to_rad(data[1]);
        }
        else
            g_foccore.pos_ref = deg_to_rad(data[0]);
        break;
    default:
        break;
    }
}
void CONTROL_mode_updata(u8 mode)
{
    g_foccore.loop_mode = mode;
}