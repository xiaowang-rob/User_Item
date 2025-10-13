#include "loop_control.h"
#include "string.h"
#include "system_parameters.h"
#include "math_fast.h"
LOOP_CON_t g_loop_con = {0};

void Frequency_division_init(float fspeed, float fpos)
{
    memset(&g_loop_con.fd, 0, sizeof(f_Division_t));
    g_loop_con.fd.fspeed_loop = fspeed;
    g_loop_con.fd.fposition_loop = fpos;
    g_loop_con.fd.speed_updata_steps = fpwm / fspeed;
    g_loop_con.fd.position_updata_steps = fspeed / fpos;
    g_loop_con.fd.Tspd = Tpwm * g_loop_con.fd.speed_updata_steps;
    g_loop_con.fd.Tpos = g_loop_con.fd.Tspd * g_loop_con.fd.position_updata_steps;
}
void PI_init(PI_t *pi, float kp, float ki, float output_limit)
{
    memset(pi, 0, sizeof(PI_t));
    pi->kp = kp;
    pi->ki = ki;
    pi->integral_limit = output_limit;
    pi->output_limit = output_limit;
}
void PID_init(PID_t *pid, float kp, float ki, float kd, float output_limit)
{
    memset(pid, 0, sizeof(PID_t));
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral_limit = output_limit;
    pid->output_limit = output_limit;
}
void Frequency_division_updatta()
{
    g_loop_con.fd.speed_updata = false;
    g_loop_con.fd.position_updata = false;
    g_loop_con.fd.current_steps++;
    if (g_loop_con.fd.current_steps >= g_loop_con.fd.speed_updata_steps)
    {
        g_loop_con.fd.current_steps = 0;
        g_loop_con.fd.speed_updata = true;
        g_loop_con.fd.speed_steps++;
        if (g_loop_con.fd.speed_steps >= g_loop_con.fd.position_updata_steps)
        {
            g_loop_con.fd.speed_steps = 0;
            g_loop_con.fd.position_updata = true;
        }
    }
}
void Frequency_division_reset()
{
    g_loop_con.fd.current_steps = 0;
    g_loop_con.fd.speed_steps = 0;
}
// PI控制器更新
float PI_updata(PI_t *pi, float ref, float fb, float dt)
{
    float error = ref - fb;

    if (pi->enable_integral)
    {
        pi->integral += error * dt;

        // 积分限幅
        if (pi->integral > pi->integral_limit)
        {
            pi->integral = pi->integral_limit;
        }
        else if (pi->integral < -pi->integral_limit)
        {
            pi->integral = -pi->integral_limit;
        }
    }

    // 计算输出
    pi->output = pi->kp * error + pi->ki * pi->integral;
    pi->enable_integral = true;
    // 输出限幅
    if (pi->output > pi->output_limit)
    {
        pi->output = pi->output_limit;
        pi->enable_integral = false;
    }
    else if (pi->output < -pi->output_limit)
    {
        pi->output = -pi->output_limit;
        pi->enable_integral = false;
    }
    return pi->output;
}
// PID控制器更新
float PID_update(PID_t *pid, float ref, float fb, float dt)
{
    float error = ref - fb;

    // 积分项
    if (pid->enable_integral)
    {
        pid->integral += error * dt;

        // 积分限幅
        if (pid->integral > pid->integral_limit)
        {
            pid->integral = pid->integral_limit;
        }
        else if (pid->integral < -pid->integral_limit)
        {
            pid->integral = -pid->integral_limit;
        }
    }

    // 微分项
    float d_term = 0.0f;

    float derivative = (error - pid->last_error) / dt;

    // 应用滤波器
    pid->derivative = pid->derivative_filter * derivative +
                      (1.0f - pid->derivative_filter) * pid->last_derivative;

    // 微分限幅
    if (pid->derivative > pid->derivative_limit)
    {
        pid->derivative = pid->derivative_limit;
    }
    else if (pid->derivative < -pid->derivative_limit)
    {
        pid->derivative = -pid->derivative_limit;
    }

    pid->last_derivative = pid->derivative;
    pid->output = pid->kp * error + pid->ki * pid->integral + pid->kd * pid->derivative;
    pid->enable_integral = true;
    // 输出限幅
    if (pid->output > pid->output_limit)
    {
        pid->output = pid->output_limit;
        pid->enable_integral = false;
    }
    else if (pid->output < -pid->output_limit)
    {
        pid->output = -pid->output_limit;
        pid->enable_integral = false;
    }

    // 保存上一次的误差
    pid->last_error = error;

    return pid->output;
}
void PI_reset(PI_t *pi)
{
    pi->integral = 0.0f;
    pi->output = 0.0f;
    pi->enable_integral = false;
}
void PID_reset(PID_t *pid)
{
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->last_error = 0.0f;
    pid->last_derivative = 0.0f;
    pid->output = 0.0f;
    pid->enable_integral = false;
}
void loop_reset(void)
{
    PI_reset(&g_loop_con.PI_id);
    PI_reset(&g_loop_con.PI_iq);
    PI_reset(&g_loop_con.PI_speed);
    PID_reset(&g_loop_con.PID_pos);
}

void LOOP_Parameter_writing(float fspeed, float fpos, float Udc, float max_current, float max_speed, float position_min, float position_max, float kp_id, float ki_id,
                            float kp_iq, float ki_iq, float kp_speed, float ki_speed, float kp_pos, float ki_pos, float kd_pos)
{
    Frequency_division_init(fspeed, fpos);
    PI_init(&g_loop_con.PI_id, kp_id, ki_id, M_PI * Udc / 2);
    PI_init(&g_loop_con.PI_iq, kp_iq, ki_iq, M_PI * Udc / 2);
    PI_init(&g_loop_con.PI_speed, kp_speed, ki_speed, max_current);
    PID_init(&g_loop_con.PID_pos, kp_pos, ki_pos, kd_pos, max_speed);
    g_loop_con.position_min = position_min;
    g_loop_con.position_max = position_max;
}
float Current_loop(float current_ref, float current_fb)
{
    return PI_updata(&g_loop_con.PI_iq, current_ref, current_fb, Tpwm);
}
float Magnetic_loop(float id_ref, float id_fb)
{
    return PI_updata(&g_loop_con.PI_id, id_ref, id_fb, Tpwm);
}
float Speed_loop(float omega_ref, float omega_fb)
{
    return PI_updata(&g_loop_con.PI_speed, omega_ref, omega_fb, g_loop_con.fd.Tspd);
}
float Position_abs_loop(float position_ref, float position_fb)
{
    return PID_update(&g_loop_con.PID_pos, position_ref, position_fb, g_loop_con.fd.Tpos);
}
float Position_rel_loop(float position_ref, float position_fb)
{
    if (position_ref > g_loop_con.position_max)
        position_ref = g_loop_con.position_max;
    if (position_ref < g_loop_con.position_min)
        position_ref = g_loop_con.position_min;
    return PID_update(&g_loop_con.PID_pos, position_ref, position_fb, g_loop_con.fd.Tpos);
}
