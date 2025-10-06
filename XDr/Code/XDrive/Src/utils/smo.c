#include "smo.h"
#include "math_fast.h"
#include "system_parameters.h"
/*无感SMO观测器*/
void smo_sensorless_init(smo_sensorless_t *smo, float Rs, float Ls, float dt)
{
    smo->Rs = Rs;
    smo->Ls = Ls;
    smo->dt = dt;
    smo->k_sl = 100000.0f / fpwm;
    smo->k_f = smo->k_sl * 10;

    smo->i_alpha_hat = 0.0f;
    smo->i_beta_hat = 0.0f;
    smo->e_alpha = 0.0f;
    smo->e_beta = 0.0f;
    smo->e_alpha_filtered = 0.0f;
    smo->e_beta_filtered = 0.0f;
    smo->theta = 0.0f;
    smo->omega = 0.0f;
    smo->theta_prev = 0.0f;
}

void smo_sensorless_update(smo_sensorless_t *smo,
                           float v_alpha, float v_beta,
                           float i_alpha, float i_beta)
{
    // 估算电流
    smo->i_alpha_hat += ((v_alpha - smo->Rs * smo->i_alpha_hat - smo->e_alpha) / smo->Ls) * smo->dt;
    smo->i_beta_hat += ((v_beta - smo->Rs * smo->i_beta_hat - smo->e_beta) / smo->Ls) * smo->dt;

    // 滑模面
    float i_alpha_error = i_alpha - smo->i_alpha_hat;
    float i_beta_error = i_beta - smo->i_beta_hat;

    // 滑模切换函数
    float sign_alpha = (i_alpha_error > 0) ? 1.0f : -1.0f;
    float sign_beta = (i_beta_error > 0) ? 1.0f : -1.0f;

    // 估算反电动势
    smo->e_alpha = smo->k_sl * sign_alpha;
    smo->e_beta = smo->k_sl * sign_beta;

    // 低通滤波
    smo->e_alpha_filtered += smo->k_f * (smo->e_alpha - smo->e_alpha_filtered) * smo->dt;
    smo->e_beta_filtered += smo->k_f * (smo->e_beta - smo->e_beta_filtered) * smo->dt;

    // 计算角度
    smo->theta = atan2f(smo->e_beta_filtered, smo->e_alpha_filtered);
    // 计算速度（角度微分）
    float speed_raw = (smo->theta - smo->theta_prev) / smo->dt;
    smo->theta_prev = smo->theta;

    // 处理角度跳变
    if (speed_raw > M_PI)
        speed_raw -= M2_PI;
    else if (speed_raw < -M_PI)
        speed_raw += M2_PI;

    smo->omega = speed_raw;
}

float smo_sensorless_get_theta(smo_sensorless_t *smo)
{
    return smo->theta;
}

float smo_sensorless_get_omega(smo_sensorless_t *smo)
{
    return smo->omega;
}

/*SMO整定器*/
void param_tuning_init(param_tuning_t *smo,
                       float initial_Rs, float initial_Ls,
                       float initial_Psi_f, float initial_pole_pairs,
                       float dt)
{
    smo->Rs = initial_Rs;
    smo->Ls = initial_Ls;
    smo->Psi_f = initial_Psi_f;
    smo->pole_pairs = initial_pole_pairs;
    smo->J = 0.001f;
    smo->B = 0.001f;
    smo->dt = dt;
    smo->k_sl = 100000.0f / fpwm;
    smo->k_f = smo->k_sl * 10;
    smo->torque_constant = 0.1f; // 初始化转矩常数

    smo->i_alpha_hat = 0.0f;
    smo->i_beta_hat = 0.0f;
    smo->e_alpha = 0.0f;
    smo->e_beta = 0.0f;
    smo->e_alpha_filtered = 0.0f;
    smo->e_beta_filtered = 0.0f;

    smo->tune_state = PARAM_TUNE_IDLE;
    smo->tune_samples = 0;

    memset(smo->i_history, 0, sizeof(smo->i_history));
    memset(smo->v_history, 0, sizeof(smo->v_history));
    memset(smo->speed_history, 0, sizeof(smo->speed_history));
    smo->history_index = 0;

    smo->Rs_updated = false;
    smo->Ls_updated = false;
    smo->Psi_f_updated = false;
    smo->pole_pairs_updated = false;
    smo->J_updated = false;
    smo->B_updated = false;
}
float omega_elec_last = 0;
// 无感整定更新
void sensorless_param_tuning_update(param_tuning_t *smo,
                                    float v_alpha, float v_beta,
                                    float i_alpha, float i_beta, float omega_electrical)
{
    // SMO 电流估算器
    smo->i_alpha_hat += ((v_alpha - smo->Rs * smo->i_alpha_hat - smo->e_alpha) / smo->Ls) * smo->dt;
    smo->i_beta_hat += ((v_beta - smo->Rs * smo->i_beta_hat - smo->e_beta) / smo->Ls) * smo->dt;

    // 滑模面
    float i_alpha_error = i_alpha - smo->i_alpha_hat;
    float i_beta_error = i_beta - smo->i_beta_hat;

    // 滑模切换函数（带滞环）
    float hysteresis = 0.01f;
    float sign_alpha = (i_alpha_error > hysteresis) ? 1.0f : (i_alpha_error < -hysteresis) ? -1.0f
                                                                                           : 0.0f;
    float sign_beta = (i_beta_error > hysteresis) ? 1.0f : (i_beta_error < -hysteresis) ? -1.0f
                                                                                        : 0.0f;

    // 反电动势估算
    smo->e_alpha = smo->k_sl * sign_alpha;
    smo->e_beta = smo->k_sl * sign_beta;

    // 低通滤波
    smo->e_alpha_filtered += smo->k_f * (smo->e_alpha - smo->e_alpha_filtered) * smo->dt;
    smo->e_beta_filtered += smo->k_f * (smo->e_beta - smo->e_beta_filtered) * smo->dt;

    // 参数整定状态机
    switch (smo->tune_state)
    {
    case PARAM_TUNE_RS:
        if (tune_resistance_correct(smo, v_alpha, v_beta, i_alpha, i_beta))
            smo->tune_state = PARAM_TUNE_LS;
        break;

    case PARAM_TUNE_LS:
        if (tune_inductance_correct(smo, v_alpha, v_beta, i_alpha, i_beta))
            smo->tune_state = PARAM_TUNE_FLUX;
        break;

    case PARAM_TUNE_FLUX:
        if ((omega_electrical - omega_elec_last > 0.1) || omega_electrical == 0)
            break;
        if (tune_flux_correct(smo, omega_electrical))
            smo->tune_state = PARAM_TUNE_COMPLETE;
        break;
    default:
        break;
    }
    omega_elec_last = omega_electrical;
}
// 有感整定更新
static float omega_last = 0;
void encoder_param_tuning_update(param_tuning_t *smo,
                                 float v_alpha_applied, float v_beta_applied,
                                 float i_alpha, float i_beta,
                                 float theta_mechanical, float omega_mechanical)
{
    float omega_electrical = 0;
    float theta_electrical = 0;
    smo->i_alpha_hat += ((v_alpha_applied - smo->Rs * smo->i_alpha_hat - smo->e_alpha) / smo->Ls) * smo->dt;
    smo->i_beta_hat += ((v_beta_applied - smo->Rs * smo->i_beta_hat - smo->e_beta) / smo->Ls) * smo->dt;

    float i_alpha_error = i_alpha - smo->i_alpha_hat;
    float i_beta_error = i_beta - smo->i_beta_hat;

    float hysteresis = 0.01f;
    float sign_alpha = (i_alpha_error > hysteresis) ? 1.0f : (i_alpha_error < -hysteresis) ? -1.0f
                                                                                           : 0.0f;
    float sign_beta = (i_beta_error > hysteresis) ? 1.0f : (i_beta_error < -hysteresis) ? -1.0f
                                                                                        : 0.0f;

    smo->e_alpha = smo->k_sl * sign_alpha;
    smo->e_beta = smo->k_sl * sign_beta;

    smo->e_alpha_filtered += smo->k_f * (smo->e_alpha - smo->e_alpha_filtered) * smo->dt;
    smo->e_beta_filtered += smo->k_f * (smo->e_beta - smo->e_beta_filtered) * smo->dt;

    // 参数整定状态机

    switch (smo->tune_state)
    {
    case PARAM_TUNE_THETA_OFFSET:
        if (omega_mechanical > 0.01f)
            break;
        smo->theta_offset = theta_mechanical;
        smo->theta_offset_updated = true;
        smo->tune_state = PARAM_TUNE_POLE_PAIRS;
        break;
    case PARAM_TUNE_POLE_PAIRS:
        if (omega_mechanical - omega_last > 1)
            break;
        if (tune_pole_pairs_correct(smo, theta_mechanical))
            smo->tune_state = PARAM_TUNE_RS;
        break;

    case PARAM_TUNE_RS:
        if (omega_mechanical > 0.01f)
            break;
        if (tune_resistance_correct(smo, v_alpha_applied, v_beta_applied, i_alpha, i_beta))
            smo->tune_state = PARAM_TUNE_LS;
        break;

    case PARAM_TUNE_LS:
        if (omega_mechanical > 0.01f)
            break;
        if (tune_inductance_correct(smo, v_alpha_applied, v_beta_applied, i_alpha, i_beta))
            smo->tune_state = PARAM_TUNE_FLUX;
        break;

    case PARAM_TUNE_FLUX:
        if (omega_mechanical - omega_last > 0.1)
            break;
        omega_electrical = omega_mechanical * smo->pole_pairs;
        if (tune_flux_correct(smo, omega_electrical))
            smo->tune_state = PARAM_TUNE_INERTIA;
        break;

    case PARAM_TUNE_INERTIA:
        theta_electrical = theta_mechanical * smo->pole_pairs;
        if (tune_inertia_correct(smo, (omega_mechanical - omega_last) / smo->dt, i_alpha, i_beta, theta_electrical))
            smo->tune_state = PARAM_TUNE_FRICTION;
        break;

    case PARAM_TUNE_FRICTION:
        if (tune_friction_correct(smo, omega_mechanical, i_alpha, i_beta))
            smo->tune_state = PARAM_TUNE_COMPLETE;
        break;

    default:
        break;
    }
    omega_last = omega_mechanical;
}

// 1. 极对数整定

bool tune_pole_pairs_correct(param_tuning_t *smo, float theta_mechanical)
{
    static float mech_prev = 0.0f;
    static float elec_prev = 0.0f;

    float theta_elec = atan2f(smo->e_beta_filtered, smo->e_alpha_filtered);

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
            smo->pole_pairs = 0.9f * smo->pole_pairs + 0.1f * new_pole_pairs;
            smo->tune_samples++;
        }
    }

    mech_prev = theta_mechanical;
    elec_prev = theta_elec;

    if (smo->tune_samples >= 100)
    {
        smo->pole_pairs_updated = true;
        smo->tune_samples = 0;
        return true;
    }
    return false;
}

// 2. 电阻整定
bool tune_resistance_correct(param_tuning_t *smo, float v_alpha, float v_beta,
                             float i_alpha, float i_beta)
{
    float v_mag = sqrtf(v_alpha * v_alpha + v_beta * v_beta);
    float i_mag = sqrtf(i_alpha * i_alpha + i_beta * i_beta);

    if (i_mag > 0.05f && i_mag < 1.0f && v_mag > 0.1f && v_mag < 5.0f &&
        smo->tune_samples < 200)
    {

        float new_Rs = v_mag / i_mag;
        if (new_Rs > 0.001f && new_Rs < 5.0f)
        {
            smo->Rs = 0.98f * smo->Rs + 0.02f * new_Rs;
            smo->tune_samples++;
        }
    }

    if (smo->tune_samples >= 200)
    {
        if (smo->Rs > 0.01f && smo->Rs < 5.0f)
            smo->fault_flag = true;
        smo->Rs_updated = true;
        smo->tune_samples = 0;
        return true;
    }
    return false;
}

// 3. 电感整定
bool tune_inductance_correct(param_tuning_t *smo, float v_alpha, float v_beta,
                             float i_alpha, float i_beta)
{
    static float i_alpha_prev = 0.0f;
    static float i_beta_prev = 0.0f;

    float di_alpha_dt = (i_alpha - i_alpha_prev) / smo->dt;
    float di_beta_dt = (i_beta - i_beta_prev) / smo->dt;
    float di_dt = sqrtf(di_alpha_dt * di_alpha_dt + di_beta_dt * di_beta_dt);

    float v_alpha_comp = v_alpha - smo->Rs * i_alpha;
    float v_beta_comp = v_beta - smo->Rs * i_beta;
    float v_comp = sqrtf(v_alpha_comp * v_alpha_comp + v_beta_comp * v_beta_comp);

    if (di_dt > 5.0f && v_comp > 0.1f && smo->tune_samples < 100)
    {
        float new_Ls = v_comp / di_dt;
        if (new_Ls > 0.000001f && new_Ls < 0.1f)
        {
            smo->Ls = 0.9f * smo->Ls + 0.1f * new_Ls;
            smo->tune_samples++;
        }
    }

    i_alpha_prev = i_alpha;
    i_beta_prev = i_beta;

    if (smo->tune_samples >= 100)
    {
        if (smo->Ls > 0.000005f && smo->Ls < 0.0002f)
            smo->fault_flag = true;
        smo->Ls_updated = true;
        smo->tune_samples = 0;
        return true;
    }
    return false;
}

// 4. 磁链整定
bool tune_flux_correct(param_tuning_t *smo, float omega_electrical)
{
    if (fast_absf(omega_electrical) > 10.0f && fast_absf(omega_electrical) < 300.0f &&
        smo->tune_samples < 200)
    {

        float e_mag = sqrtf(smo->e_alpha_filtered * smo->e_alpha_filtered +
                            smo->e_beta_filtered * smo->e_beta_filtered);

        if (fast_absf(omega_electrical) > 1.0f)
        {
            float new_Psi_f = e_mag / fast_absf(omega_electrical);
            if (new_Psi_f > 0.0001f && new_Psi_f < 1.0f)
            {
                smo->Psi_f = 0.9f * smo->Psi_f + 0.1f * new_Psi_f;
                smo->tune_samples++;
            }
        }
    }

    if (smo->tune_samples >= 200)
    {
        smo->Psi_f_updated = true;
        smo->tune_samples = 0;
        return true;
    }
    return false;
}

// 5. 惯量整定（物理正确版）
bool tune_inertia_correct(param_tuning_t *smo, float acceleration,
                          float i_alpha, float i_beta, float theta_electrical)
{
    // 计算转矩（基于当前参数）
    // T = 1.5 * P * [Ψf * iq + (Ld - Lq) * id * iq]
    // 简化：假设 Ld ≈ Lq，主要为永磁转矩
    float id = i_alpha * arm_cos_f32(theta_electrical) + i_beta * arm_sin_f32(theta_electrical);
    float iq = -i_alpha * arm_sin_f32(theta_electrical) + i_beta * arm_cos_f32(theta_electrical);

    float torque = 1.5f * smo->pole_pairs * (smo->Psi_f * iq); // 简化模型

    if (fast_absf(acceleration) > 0.1f && fast_absf(acceleration) < 50.0f &&
        fast_absf(torque) > 0.01f && smo->tune_samples < 100)
    {

        float new_J = torque / acceleration;
        if (new_J > 0.000001f && new_J < 0.1f)
        {
            smo->J = 0.9f * smo->J + 0.1f * new_J;
            smo->tune_samples++;
        }
    }

    if (smo->tune_samples >= 100)
    {
        smo->J_updated = true;
        smo->tune_samples = 0;
        return true;
    }
    return false;
}

// 6. 摩擦系数整定（物理正确版）
bool tune_friction_correct(param_tuning_t *smo, float omega_mechanical,
                           float i_alpha, float i_beta)
{
    static float time_prev = 0.0f;

    if (smo->tune_samples < 200)
    {
        if (fast_absf(omega_mechanical) < 0.5f && fast_absf(omega_mechanical) > 0.01f)
        {
            // 低速稳态，电流主要用于克服摩擦
            float id = i_alpha * arm_cos_f32(0) + i_beta * arm_sin_f32(0); // 简化：假设角度为0
            float iq = -i_alpha * arm_sin_f32(0) + i_beta * arm_cos_f32(0);

            // 转矩 = Kt * iq
            float torque_total = 1.5f * smo->pole_pairs * smo->Psi_f * iq;

            // 假设总转矩用于克服摩擦（忽略负载）
            float torque_friction = torque_total;
            float new_B = torque_friction / fast_absf(omega_mechanical);

            if (new_B > 0.0001f && new_B < 0.1f)
            {
                smo->B = 0.9f * smo->B + 0.1f * new_B;
                smo->tune_samples++;
            }
        }
    }

    if (smo->tune_samples >= 200)
    {
        smo->B_updated = true;
        smo->tune_samples = 0;
        return true;
    }
    return false;
}

param_tune_state_t param_tuning_get_state(param_tuning_t *smo)
{
    return smo->tune_state;
}

float param_tuning_get_pole_pairs(param_tuning_t *smo)
{
    return smo->pole_pairs;
}

float param_tuning_get_Rs(param_tuning_t *smo)
{
    return smo->Rs;
}

float param_tuning_get_Ls(param_tuning_t *smo)
{
    return smo->Ls;
}

float param_tuning_get_Psi_f(param_tuning_t *smo)
{
    return smo->Psi_f;
}

float param_tuning_get_J(param_tuning_t *smo)
{
    return smo->J;
}

float param_tuning_get_B(param_tuning_t *smo)
{
    return smo->B;
}
