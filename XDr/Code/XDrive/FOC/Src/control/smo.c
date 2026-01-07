#include "smo.h"
#include "math_fast.h"
#include "system_parameters.h"
#include "foc_core.h"
#include "encoder.h"

smo_t smo;
smo_t *get_smo_adr()
{
    return &smo;
}
/*无感SMO观测器*/
void smo_init(float Rs, float Ls, float Psi_f, float max_speed, float pole_pairs,
              float Ke, float J, float B)
{
    smo.Rs = Rs;
    smo.Ls = Ls;
    smo.Psi_f = Psi_f;
    smo.pole_pairs = pole_pairs;
    smo.J = J;
    smo.B = B;
    smo.Ke = Ke;
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
// pwm的1/4频率跑
#define TUN_f fpwm / 4

#define THETA_OFFSET_samples 2000                      // 0.4s
#define THETA_OFFSET_timeout THETA_OFFSET_samples * 10 // 4s

#define RS_samples 1000            // 0.2s
#define RS_timeout RS_samples * 10 // 2s

#define Ls_inject_f 10
#define Ls_inject_u 3
#define Ls_tic TUN_f / Ls_inject_f
#define Ls_samples 1000            // 0.2s
#define Ls_timeout Ls_samples * 10 // 2s

#define POLE_PAIRS_omega 10                        // 500rpm
#define POLE_PAIRS_samples 10000                   // 2s
#define POLE_PAIRS_timeout POLE_PAIRS_samples * 10 // 20s

#define PK_samples 10000           // 2s
#define PK_timeout PK_samples * 10 // 20s

param_tuning_t tun = {0};
param_tuning_t *get_tuning_adr()
{
    return &tun;
}

void param_tuning_init()
{
    tun.tune_state = PARAM_TUNE_IDLE;
    tun.tune_samples = 0;

    tun.fault_flag = false;
}
// 1.角度偏移整定
static bool param_tune_theta_offset(float theta_mech)
{
    // 稳态点采样
    if (theta_mech - tun.theta_mech_prev < 0.01f)
    {
        tun.tune_samples++; // 稳态计数
        if (tun.tune_samples > THETA_OFFSET_samples - 100)
        { // 最后100个求平均
            smo.theta_offset += theta_mech * 0.01f;
        }
        else
        {
            smo.theta_offset = 0.0f;
        }
    }
    else
    {
        tun.tune_samples = 0;
    }
    tun.theta_mech_prev = theta_mech;
    // 完成
    if (tun.tune_samples >= THETA_OFFSET_samples)
    {
        SET_ENCODER_ANGLE_OFFSET(smo.theta_offset);
        return true;
    }
    return false;
}
// 2. 电阻整定
static bool param_tune_Rs(float v_alpha, float i_alpha)
{

    static float prev_i_alpha = 0.0f;

    if (fabsf(i_alpha - prev_i_alpha) < 0.5f)
    {
        tun.tune_samples++;
    }
    else
    {
        tun.tune_samples = 0;
    }
    prev_i_alpha = i_alpha;

    if (tun.tune_samples > RS_samples - 100)
    {
        tun.steady_i += i_alpha * 0.01f;
        tun.steady_v += v_alpha * 0.01f;
    }
    if (tun.tune_samples > RS_samples)
    {
        smo.Rs = tun.steady_v / tun.steady_i;
        if (smo.Rs > 10.0f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_HIGH;
            tun.fault_state = PARAM_TUNE_RS;
        }
        if (smo.Rs < 0.01f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_LOW;
            tun.fault_state = PARAM_TUNE_RS;
        }
        return true;
    }
    return false;
}
// 3. 电感整定
static bool param_tune_Ls(float v_alpha, float v_beta, float i_alpha, float i_beta)
{
    static float prev_i_alpha = 0.0f;
    static float prev_i_beta = 0.0f;

    // 正向方波阶段（两个轴同时为正）
    if (v_alpha > Ls_inject_u * 0.8f && v_beta > Ls_inject_u * 0.8f)
    {
        float di_alpha = (i_alpha - prev_i_alpha) / tun.dt;
        float di_beta = (i_beta - prev_i_beta) / tun.dt;
        // alpha轴：电流应上升
        if (di_alpha > 0.1f * MAX_Current / tun.dt)
        {
            tun.sum_di_dt_alpha_pos += di_alpha;
            tun.alpha_pos_count++;
        }
        // beta轴：电流应上升（同步注入，应有相似变化率）
        if (di_beta > 0.1f * MAX_Current / tun.dt)
        {
            tun.sum_di_dt_beta_pos += di_beta;
            tun.beta_pos_count++;
        }
    }
    // 负向方波阶段（两个轴同时为负）
    else if (v_alpha < -Ls_inject_u * 0.8f && v_beta < -Ls_inject_u * 0.8f)
    {
        float di_alpha = (i_alpha - prev_i_alpha) / tun.dt;
        float di_beta = (i_beta - prev_i_beta) / tun.dt;
        // alpha轴：电流应下降
        if (di_alpha < -0.1f * MAX_Current / tun.dt)
        {
            tun.sum_di_dt_alpha_neg += di_alpha;
            tun.alpha_neg_count++;
        }
        // beta轴：电流应下降
        if (di_beta < -0.1f * MAX_Current / tun.dt)
        {
            tun.sum_di_dt_beta_neg += di_beta;
            tun.beta_neg_count++;
        }
    }

    // === 完成判断 ===
    if (tun.tune_samples >= Ls_samples)
    {
        // 验证数据有效性（两个轴都需要足够样本）
        bool alpha_valid = (tun.alpha_pos_count >= 20 && tun.alpha_neg_count >= 20);
        bool beta_valid = (tun.beta_pos_count >= 20 && tun.beta_neg_count >= 20);

        if (!alpha_valid && !beta_valid)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_LS;
            return;
        }
        // === 计算alpha轴电感 ===
        float Ls_alpha = 0.0f;
        if (alpha_valid)
        {
            float avg_di_dt_alpha_pos = tun.sum_di_dt_alpha_pos / tun.alpha_pos_count;
            float avg_di_dt_alpha_neg = tun.sum_di_dt_alpha_neg / tun.alpha_neg_count;
            float avg_di_dt_alpha = (fabsf(avg_di_dt_alpha_pos) + fabsf(avg_di_dt_alpha_neg)) / 2.0f;
            Ls_alpha = Ls_inject_u / avg_di_dt_alpha;
        }

        // === 计算beta轴电感 ===
        float Ls_beta = 0.0f;
        if (beta_valid)
        {
            float avg_di_dt_beta_pos = tun.sum_di_dt_beta_pos / tun.beta_pos_count;
            float avg_di_dt_beta_neg = tun.sum_di_dt_beta_neg / tun.beta_neg_count;
            float avg_di_dt_beta = (fabsf(avg_di_dt_beta_pos) + fabsf(avg_di_dt_beta_neg)) / 2.0f;
            Ls_beta = Ls_inject_u / avg_di_dt_beta;
        }

        // === 融合两个轴的结果 ===
        if (alpha_valid && beta_valid)
        {
            // 两个轴都有效：取平均（提高精度）
            smo.Ls = (Ls_alpha + Ls_beta) / 2.0f;

            // 一致性检查：如果差异过大，可能是电机问题
            if (fabsf(Ls_alpha - Ls_beta) > 0.3f * smo.Ls)
            {
                // 差异>30%，记录警告但不报错（可用于诊断）
                tun.fault_flag = true;
                tun.fault_type = PARAM_FAULT_UNBALANCE;
                tun.fault_state = PARAM_TUNE_LS;
            }
        }
        else if (alpha_valid)
        {
            // 只有alpha轴有效
            smo.Ls = Ls_alpha;
        }
        else
        {
            // 只有beta轴有效
            smo.Ls = Ls_beta;
        }
        return true;
        if (smo.Ls < 0.001f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_LOW;
            tun.fault_state = PARAM_TUNE_LS;
            return true;
        }
        if (smo.Ls > 0.01f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_HIGH;
            tun.fault_state = PARAM_TUNE_LS;
            return true;
        }
    }
    return false;
}

// 4. 极对数整定
static bool param_tune_pole_pairs(float omega_mech, u8 pole_pairs_input)
{
    static float omega_encoder_last = 0;
    static float omega_smo_last = 0;
    static float pole_pairs = 0;
    if (omega_mech - omega_encoder_last < 0.1f && smo.omega - omega_smo_last < 1.f)
    {
        tun.tune_samples++;
    }
    else
        tun.tune_samples = 0;

    if (tun.tune_samples > POLE_PAIRS_samples - 100)
    {
        pole_pairs += (smo.omega / omega_mech) * 0.01f;
    }
    if (tun.tune_samples >= POLE_PAIRS_samples)
    {
        if (pole_pairs < 1.0f || pole_pairs > 16.0f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_POLE_PAIRS_INVALID;
            tun.fault_state = PARAM_TUNE_POLE_PAIRS;
        }
        smo.pole_pairs = (u32)(pole_pairs + 0.5f);
        if (smo.pole_pairs != pole_pairs_input)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_POLE_PAIRS_MISMATCH;
            tun.fault_state = PARAM_TUNE_POLE_PAIRS;
        }
        return true;
    }
    return false;
}

// 5. 磁链整定
static void param_tune_Psi_f(float encoder_speed, float v_alpha, float v_beta,
                             float i_alpha, float i_beta)
{
    if (tun.tune_samples == 1)
    {
        tun.max_samples = 800;
        tun.sum_e_mag = 0.0f;
        tun.sum_speed = 0.0f;

        smo_ptr->e_alpha_filtered = 0.0f;
        smo_ptr->e_beta_filtered = 0.0f;
    }

    float speed_rpm = encoder_speed * 60.0f / (2.0f * M_PI);
    float speed_electrical = encoder_speed * smo_ptr->pole_pairs;

    if (fabsf(speed_rpm) < 200.0f)
    {
        tun.fault_flag = true;
        tun.fault_type = PARAM_FAULT_PSI_F_SPEED_LOW;
        return;
    }

    smo_update(v_alpha, v_beta, i_alpha, i_beta);

    float e_alpha = smo_ptr->e_alpha_filtered;
    float e_beta = smo_ptr->e_beta_filtered;
    float e_mag = sqrtf(e_alpha * e_alpha + e_beta * e_beta);

    if (tun.tune_samples > 200)
    {
        if (fabsf(e_mag) > 0.5f && fabsf(speed_electrical) > 100.0f)
        {
            tun.sum_e_mag += e_mag;
            tun.sum_speed += fabsf(speed_electrical);
        }
    }

    if (tun.tune_samples >= tun.max_samples)
    {
        int valid_samples = tun.tune_samples - 200;
        if (valid_samples < 100)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_PSI_F_NO_VALID_DATA;
            return;
        }

        float avg_e_mag = tun.sum_e_mag / valid_samples;
        float avg_speed = tun.sum_speed / valid_samples;

        tun.tuned_Psi_f = avg_e_mag / avg_speed;
        tun.tuned_Psi_f = safe_limit_fault(tun.tuned_Psi_f, 0.01f, 1.0f,
                                           PARAM_FAULT_PSI_F_LOW, PARAM_FAULT_PSI_F_HIGH);

        if (tun.fault_flag)
            return;

        float Ke = tun.tuned_Psi_f * smo_ptr->pole_pairs;
        smo_ptr->Psi_f = tun.tuned_Psi_f;
        smo_ptr->Ke = Ke;

        tun.tune_state = PARAM_TUNE_JB;
        tun.tune_samples = 0;
    }
}

// 6. J/B整定 - 只需要encoder_speed和torque_fb！
static void param_tune_JB(float encoder_speed, float torque_fb)
{
    if (tun.tune_samples == 1)
    {
        tun.max_samples = 1000;
        tun.step_triggered = false;
        tun.sum_accel = 0.0f;
        tun.sum_torque = 0.0f;
    }

    static float prev_speed = 0.0f;
    float accel = (encoder_speed - prev_speed) / tun.dt;
    prev_speed = encoder_speed;

    if (!tun.step_triggered && fabsf(encoder_speed) > 2.0f)
    {
        tun.step_triggered = true;
    }

    if (!tun.step_triggered)
    {
        if (tun.tune_samples >= tun.max_samples)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_J_NO_STEP;
            return;
        }
        return;
    }

    float elapsed_time = tun.tune_samples * tun.dt;

    if (elapsed_time > 0.2f && elapsed_time < 1.0f && fabsf(accel) > 1.0f)
    {
        tun.sum_accel += accel;
        tun.sum_torque += torque_fb;
    }

    if (tun.tune_samples >= tun.max_samples)
    {
        float accel_samples = 160.0f;
        if (accel_samples < 50.0f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_INSUFFICIENT_DATA;
            return;
        }

        float avg_accel = tun.sum_accel / accel_samples;
        float avg_torque = tun.sum_torque / accel_samples;

        if (fabsf(avg_accel) < 5.0f)
        {
            tun.tuned_J = 0.001f;
        }
        else
        {
            tun.tuned_J = avg_torque / avg_accel;
            tun.tuned_J = safe_limit_fault(tun.tuned_J, 0.0001f, 0.1f,
                                           PARAM_FAULT_J_LOW, PARAM_FAULT_J_HIGH);
            if (tun.fault_flag)
                return;
        }

        if (elapsed_time > 4.5f && fabsf(accel) < 1.0f && fabsf(encoder_speed) > 5.0f)
        {
            tun.tuned_B = (torque_fb - tun.tuned_J * 0.1f) / encoder_speed;
            if (tun.tuned_B < 0.0001f)
                tun.tuned_B = 0.001f;
        }
        else
        {
            tun.tuned_B = 0.001f;
        }

        smo_ptr->J = tun.tuned_J;
        smo_ptr->B = tun.tuned_B;

        tun.tune_state = PARAM_TUNE_COMPLETE;
    }
}

// 有感整定更新
static float omega_last = 0;
void param_tuning_update(float *theta_elec, float theta_mech, float *u_alpha, float *u_beta,
                         float i_alpha, float i_beta, float omega_mech, u8 pole_pairs_input)
{

    // 参数整定状态机
    switch (tun.tune_state)
    {
    case PARAM_TUNE_IDLE:
        // 前置条件
        FOC_SET_RUNMODE(ENCODER_CONTROL);
        FOC_SET_LOOPMODE(CURRENT_LOOP);
        float cur_iq_id[2] = {0.0f, 1.0f};
        FOC_SET_VER_VALUE(cur_iq_id);
        tun.tune_samples = 0;
        tun.time_tic = 0;
        tun.tune_state = PARAM_TUNE_THETA_OFFSET;
        break;
    case PARAM_TUNE_THETA_OFFSET:
        // 控制
        *theta_elec = 0;
        // 观测
        // 超时监测
        tun.time_tic++;
        if (tun.time_tic > THETA_OFFSET_timeout)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_THETA_OFFSET;
        }
        // 稳态点采样
        if (param_tune_theta_offset(theta_mech))
        { // 完成跳转
          // 前置条件
            FOC_SET_RUNMODE(SVPWM_CONTROL);
            *u_alpha = tun.Udc / 24.0f;
            *u_beta = 0;
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_RS; // 进入下一步
        }
        break;
    case PARAM_TUNE_RS:
        // 控制-无动态
        // 观测-超时监测
        tun.time_tic++;
        if (tun.time_tic > RS_timeout)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_RS;
        }
        // 稳态点采样
        if (param_tune_Rs(*u_alpha, i_alpha))
        { // 完成跳转
            FOC_SET_RUNMODE(ENCODER_CONTROL);
            FOC_SET_LOOPMODE(VOLTAGE_LOOP);
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_LS; // 进入下一步
        }
        break;
    case PARAM_TUNE_LS:
        // 控制
        if (tun.tune_samples % Ls_tic == 0)
        {
            tun.inject_flag = !tun.inject_flag;
        }
        *u_alpha = tun.inject_flag ? +Ls_inject_u : -Ls_inject_u;
        *u_beta = tun.inject_flag ? +Ls_inject_u : -Ls_inject_u;
        // 不用超时监测
        //  稳态点采样
        if (param_tune_Ls(*u_alpha, *u_beta, i_alpha, i_beta))
        { // 完成跳转
            FOC_SET_LOOPMODE(SPEED_LOOP);
            float omega_ref = 10;
            FOC_SET_VER_VALUE(&omega_ref);
            smo.pole_pairs = 1;
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_POLE_PAIRS; // 进入下一步
        }
        break;
    case PARAM_TUNE_POLE_PAIRS:
        // 控制
        smo_update(*u_alpha, *u_beta, i_alpha, i_beta);
        // 超时监测
        tun.time_tic++;
        if (tun.time_tic > RS_timeout)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_RS;
        }
        // 稳态点采样
        if (param_tune_pole_pairs(omega_mech, pole_pairs_input))
        { // 完成跳转
            FOC_SET_LOOPMODE(SPEED_LOOP);
            float omega_ref = 10;
            FOC_SET_VER_VALUE(&omega_ref);
            smo.pole_pairs = 1;
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_POLE_PAIRS; // 进入下一步
        }
        break;
    case PARAM_TUNE_PK:

        break;

    case PARAM_TUNE_JB:

        break;
    case PARAM_TUNE_COMPLETE:

        break;
    default:
        break;
    }

    if (tun.fault_flag)
        tun.tune_state = PARAM_TUNE_COMPLETE;
}
