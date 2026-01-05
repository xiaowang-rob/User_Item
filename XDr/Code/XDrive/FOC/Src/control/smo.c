#include "smo.h"
#include "math_fast.h"
#include "system_parameters.h"

smo_t smo;
/*无感SMO观测器*/
void smo_init(float Rs, float Ls, float Psi_f, float max_speed, float pole_pairs)
{
    smo.Rs = Rs;
    smo.Ls = Ls;
    smo.Psi_f = Psi_f;
    smo.pole_pairs = pole_pairs;
    smo.J = 0.001f;
    smo.B = 0.001f;
    smo.dt = Tcon;
    smo.k_sl = 100000.0f / fpwm;
    smo.k_f = smo.k_sl * 10;
    smo.delta = 0.1f;
    smo.max_omega = max_speed;
    smo.speed_filter_gain = 100.0f;

    smo.i_alpha_hat = 0.0f;
    smo.i_beta_hat = 0.0f;
    smo.e_alpha = 0.0f;
    smo.e_beta = 0.0f;
    smo.e_alpha_filtered = 0.0f;
    smo.e_beta_filtered = 0.0f;
    smo.theta = 0.0f;
    smo.omega = 0.0f;
    smo.theta_prev = 0.0f;
}
void smo_reset()
{
    smo.i_alpha_hat = 0.0f;
    smo.i_beta_hat = 0.0f;
    smo.e_alpha = 0.0f;
    smo.e_beta = 0.0f;
    smo.e_alpha_filtered = 0.0f;
    smo.e_beta_filtered = 0.0f;
    smo.theta = 0.0f;
    smo.omega = 0.0f;
    smo.theta_prev = 0.0f;
}
// 辅助函数：饱和函数，减少抖振
static float sat_func(float error, float delta)
{
    if (error > delta)
        return 1.0f;
    else if (error < -delta)
        return -1.0f;
    else
        return error / delta; // 线性过渡区域
}
void smo_update(float v_alpha, float v_beta,
                float i_alpha, float i_beta)
{
    // 估算电流
    smo.i_alpha_hat += ((v_alpha - smo.Rs * smo.i_alpha_hat - smo.e_alpha) / smo.Ls) * smo.dt;
    smo.i_beta_hat += ((v_beta - smo.Rs * smo.i_beta_hat - smo.e_beta) / smo.Ls) * smo.dt;

    // 滑模面
    float i_alpha_error = i_alpha - smo.i_alpha_hat;
    float i_beta_error = i_beta - smo.i_beta_hat;

    // 滑模切换函数 - 改进：使用饱和函数减少抖振
    float sign_alpha = sat_func(i_alpha_error, smo.delta); // delta为边界层厚度
    float sign_beta = sat_func(i_beta_error, smo.delta);

    // 估算反电动势
    smo.e_alpha = smo.k_sl * sign_alpha;
    smo.e_beta = smo.k_sl * sign_beta;

    // 低通滤波 - 改进滤波器设计
    float alpha_filter = 2.0f * M_PI * smo.k_f; // 截止频率
    smo.e_alpha_filtered += alpha_filter * (smo.e_alpha - smo.e_alpha_filtered) * smo.dt;
    smo.e_beta_filtered += alpha_filter * (smo.e_beta - smo.e_beta_filtered) * smo.dt;

    // 计算角度 - 添加异常处理
    if (fabsf(smo.e_alpha_filtered) < 1e-6 && fabsf(smo.e_beta_filtered) < 1e-6)
    {
        // 反电动势过小，无法计算角度，保持原值或使用其他方法
        // 这里保持原角度，实际应用中可能需要更复杂的处理
    }
    else
    {
        smo.theta = atan2f(smo.e_beta_filtered, smo.e_alpha_filtered);
    }

    // 计算速度（角度微分）- 改进：添加滤波和限幅
    float speed_raw = (smo.theta - smo.theta_prev) / smo.dt;

    // 速度限幅
    if (fabsf(speed_raw) > smo.max_omega)
    {
        speed_raw = (speed_raw > 0) ? smo.max_omega : -smo.max_omega;
    }

    // 速度滤波
    smo.omega += smo.speed_filter_gain * (speed_raw - smo.omega) * smo.dt;

    smo.theta_prev = smo.theta;

    // 角度归一化到[0, 2π]
    smo.theta = normalize_angle_0_2pi(smo.theta);
}

float smo_get_theta()
{
    return smo.theta;
}

float smo_get_omega()
{
    return smo.omega;
}
/*SMO整定器*/
param_tuning_t tun = {0};
// dt 运行周期
void param_tuning_init(
    float initial_Rs, float initial_Ls,
    float initial_Psi_f, float initial_pole_pairs)
{
    tun.tune_state = PARAM_TUNE_IDLE;
    tun.tune_samples = 0;

    tun.theta_offset_updated = false;
    tun.pole_pairs_updated = false;
    tun.RL_updated = false;
    tun.PK_updated = false;
    tun.JB_updated = false;

    tun.fault_flag = false;
}
float omega_elec_last = 0;
// 无感整定更新
void sensorless_param_tuning_update(
    float v_alpha, float v_beta,
    float i_alpha, float i_beta, float omega_electrical)
{
    // 参数整定状态机
    switch (tun.tune_state)
    {
    case PARAM_TUNE_RL:
        if (tune_resistance_correct(tun, v_alpha, v_beta, i_alpha, i_beta))
            tun.tune_state = PARAM_TUNE_PK;
        break;
    case PARAM_TUNE_PK:
        if ((omega_electrical - omega_elec_last > 0.1) || omega_electrical == 0)
            break;
        if (tune_flux_correct(tun, omega_electrical))
            tun.tune_state = PARAM_TUNE_COMPLETE;
        break;
    default:
        break;
    }
    omega_elec_last = omega_electrical;
}
// 有感整定更新
static float omega_last = 0;
void encoder_param_tuning_update(
    float v_alpha_applied, float v_beta_applied,
    float i_alpha, float i_beta,
    float theta_mechanical, float omega_mechanical)
{

    // 参数整定状态机

    switch (tun.tune_state)
    {
    case PARAM_TUNE_THETA_OFFSET:
        if (omega_mechanical > 0.01f)
            break;
        tun.theta_offset = theta_mechanical;
        tun.theta_offset_updated = true;
        tun.tune_state = PARAM_TUNE_POLE_PAIRS;
        break;
    case PARAM_TUNE_POLE_PAIRS:
        if (omega_mechanical - omega_last > 1)
            break;
        if (tune_pole_pairs_correct(tun, theta_mechanical))
            tun.tune_state = PARAM_TUNE_RS;
        break;

    case PARAM_TUNE_RL:
        if (omega_mechanical > 0.01f)
            break;
        if (tune_resistance_correct(tun, v_alpha_applied, v_beta_applied, i_alpha, i_beta))
            tun.tune_state = PARAM_TUNE_LS;
        break;
    case PARAM_TUNE_PK:
        if (omega_mechanical - omega_last > 0.1)
            break;
        omega_electrical = omega_mechanical * tun.pole_pairs;
        if (tune_flux_correct(tun, omega_electrical))
            tun.tune_state = PARAM_TUNE_JB;
        break;

    case PARAM_TUNE_JB:
        theta_electrical = theta_mechanical * tun.pole_pairs;
        if (tune_inertia_correct(tun, (omega_mechanical - omega_last) / tun.dt, i_alpha, i_beta, theta_electrical))
            tun.tune_state = PARAM_TUNE_COMPLETE;
        break;

    default:
        break;
    }
    omega_last = omega_mechanical;
}

// 1. 极对数整定

bool tune_pole_pairs_correct(float theta_mechanical)
{
    static float mech_prev = 0.0f;
    static float elec_prev = 0.0f;

    float theta_elec = atan2f(tun.e_beta_filtered, tun.e_alpha_filtered);

    float dtheta_mech = theta_mechanical - mech_prev;
    float dtheta_elec = theta_elec - elec_prev;

    if (dtheta_elec > M_PI)
        dtheta_elec -= M2_PI;
    if (dtheta_elec < -M_PI)
        dtheta_elec += M2_PI;

    if (fast_absf(dtheta_mech) > 0.01f)
    {
        float new_pole_pairs = dtheta_elec / dtheta_mech;
        new_pole_pairs = (float)fast_roundf(fast_absf(new_pole_pairs));

        if (new_pole_pairs >= 1.0f && new_pole_pairs <= 20.0f)
        {
            tun.pole_pairs = 0.9f * tun.pole_pairs + 0.1f * new_pole_pairs;
            tun.tune_samples++;
        }
    }

    mech_prev = theta_mechanical;
    elec_prev = theta_elec;

    if (tun.tune_samples >= 100)
    {
        tun.pole_pairs_updated = true;
        tun.tune_samples = 0;
        return true;
    }
    return false;
}

// 2. 电阻整定
bool tune_resistance_correct(float v_alpha, float v_beta,
                             float i_alpha, float i_beta)
{
    float v_mag = sqrtf(v_alpha * v_alpha + v_beta * v_beta);
    float i_mag = sqrtf(i_alpha * i_alpha + i_beta * i_beta);

    if (i_mag > 0.05f && i_mag < 1.0f && v_mag > 0.1f && v_mag < 5.0f &&
        tun.tune_samples < 200)
    {

        float new_Rs = v_mag / i_mag;
        if (new_Rs > 0.001f && new_Rs < 5.0f)
        {
            tun.Rs = 0.98f * tun.Rs + 0.02f * new_Rs;
            tun.tune_samples++;
        }
    }

    if (tun.tune_samples >= 200)
    {
        if (tun.Rs > 0.01f && tun.Rs < 5.0f)
            tun.fault_flag = true;
        tun.Rs_updated = true;
        tun.tune_samples = 0;
        return true;
    }
    return false;
}

// 3. 电感整定
bool tune_inductance_correct(float v_alpha, float v_beta,
                             float i_alpha, float i_beta)
{
    static float i_alpha_prev = 0.0f;
    static float i_beta_prev = 0.0f;

    float di_alpha_dt = (i_alpha - i_alpha_prev) / tun.dt;
    float di_beta_dt = (i_beta - i_beta_prev) / tun.dt;
    float di_dt = sqrtf(di_alpha_dt * di_alpha_dt + di_beta_dt * di_beta_dt);

    float v_alpha_comp = v_alpha - tun.Rs * i_alpha;
    float v_beta_comp = v_beta - tun.Rs * i_beta;
    float v_comp = sqrtf(v_alpha_comp * v_alpha_comp + v_beta_comp * v_beta_comp);

    if (di_dt > 5.0f && v_comp > 0.1f && tun.tune_samples < 100)
    {
        float new_Ls = v_comp / di_dt;
        if (new_Ls > 0.000001f && new_Ls < 0.1f)
        {
            tun.Ls = 0.9f * tun.Ls + 0.1f * new_Ls;
            tun.tune_samples++;
        }
    }

    i_alpha_prev = i_alpha;
    i_beta_prev = i_beta;

    if (tun.tune_samples >= 100)
    {
        if (tun.Ls > 0.000005f && tun.Ls < 0.0002f)
            tun.fault_flag = true;
        tun.Ls_updated = true;
        tun.tune_samples = 0;
        return true;
    }
    return false;
}

// 4. 磁链整定
bool tune_flux_correct(float omega_electrical)
{
    if (fast_absf(omega_electrical) > 10.0f && fast_absf(omega_electrical) < 300.0f &&
        tun.tune_samples < 200)
    {

        float e_mag = sqrtf(tun.e_alpha_filtered * tun.e_alpha_filtered +
                            tun.e_beta_filtered * tun.e_beta_filtered);

        if (fast_absf(omega_electrical) > 1.0f)
        {
            float new_Psi_f = e_mag / fast_absf(omega_electrical);
            if (new_Psi_f > 0.0001f && new_Psi_f < 1.0f)
            {
                tun.Psi_f = 0.9f * tun.Psi_f + 0.1f * new_Psi_f;
                tun.tune_samples++;
            }
        }
    }

    if (tun.tune_samples >= 200)
    {
        tun.Psi_f_updated = true;
        tun.tune_samples = 0;
        return true;
    }
    return false;
}

// 5. 惯量整定（物理正确版）
bool tune_inertia_correct(float acceleration,
                          float i_alpha, float i_beta, float theta_electrical)
{
    // 计算转矩（基于当前参数）
    // T = 1.5 * P * [Ψf * iq + (Ld - Lq) * id * iq]
    // 简化：假设 Ld ≈ Lq，主要为永磁转矩
    float id = i_alpha * arm_cos_f32(theta_electrical) + i_beta * arm_sin_f32(theta_electrical);
    float iq = -i_alpha * arm_sin_f32(theta_electrical) + i_beta * arm_cos_f32(theta_electrical);

    float torque = 1.5f * tun.pole_pairs * (tun.Psi_f * iq); // 简化模型

    if (fast_absf(acceleration) > 0.1f && fast_absf(acceleration) < 50.0f &&
        fast_absf(torque) > 0.01f && tun.tune_samples < 100)
    {

        float new_J = torque / acceleration;
        if (new_J > 0.000001f && new_J < 0.1f)
        {
            tun.J = 0.9f * tun.J + 0.1f * new_J;
            tun.tune_samples++;
        }
    }

    if (tun.tune_samples >= 100)
    {
        tun.JB_updated = true;
        tun.tune_samples = 0;
        return true;
    }
    return false;
}

param_tune_state_t param_tuning_get_state(param_tuning_t *tun)
{
    return tun.tune_state;
}
