#include "smo.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"

tSMO smo;

#define MAX_CURRENT_EST MAX_Current / 2.0f // 根据电机额定电流设置
#define INTEGRATOR_LIMIT 1000.0f           // 积分器限幅 (A/s)

/*无感SMO观测器*/
void fSMO_Init(tMotor motor)
{
    smo.Rs = motor.Rs;
    smo.Ls = motor.Ls;
    smo.Psi_f = motor.Psi_f;
    smo.Ke = motor.Ke;
    smo.wire_sequence = motor.Wire_sequence;
    smo.pole_pairs = motor.pole_pairs;
    smo.J = motor.J;
    smo.B = motor.B;

    smo.Udc = motor.Udc;
    smo.dt = Tcon;
    // 初始状态
    smo.i_alpha_hat = 0.0f;
    smo.i_beta_hat = 0.0f;
    smo.e_alpha = 0.0f;
    smo.e_beta = 0.0f;
    smo.e_alpha_filtered = 0.0f;
    smo.e_beta_filtered = 0.0f;
    smo.theta = 0.0f;
    smo.theta_prev = 0.0f;
    smo.omega = 0.0f;

    // 启动控制
    smo.is_aligned = false;
    smo.alignment_time = 0;
    smo.startup_gain = 0.0f;

    // 观测器参数
    smo.k_sl = 15.0f;       // 基础增益
    smo.delta = 0.1f;       // 边界层
    smo.k_f = 100.0f;       // 滤波截止频率
    smo.max_omega = 500.0f; // 500rpm电角速度

    // 积分器保护
    smo.integrator_limit = 1000.0f; // A/s
}
void fSMO_Reset()
{
    // 初始状态
    smo.i_alpha_hat = 0.0f;
    smo.i_beta_hat = 0.0f;
    smo.e_alpha = 0.0f;
    smo.e_beta = 0.0f;
    smo.e_alpha_filtered = 0.0f;
    smo.e_beta_filtered = 0.0f;
    smo.theta = 0.0f;
    smo.theta_prev = 0.0f;
    smo.omega = 0.0f;

    // 启动控制
    smo.is_aligned = false;
    smo.alignment_time = 0;
    smo.startup_gain = 0.0f;
}

float clampf(float value, float min, float max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}
float normalize_angle_pi_pi(float angle)
{
    while (angle > MATH_PI)
        angle -= MATH_2PI;
    while (angle < -MATH_PI)
        angle += MATH_2PI;
    return angle;
}
void fSMO_MainLoop(float v_alpha, float v_beta,
                   float i_alpha, float i_beta)
{
    // ✅ 阶段1：启动对齐（最关键！）
    if (!smo.is_aligned)
    {
        // 启动阶段：强制估算电流跟随实际电流
        smo.i_alpha_hat = i_alpha;
        smo.i_beta_hat = i_beta;
        smo.e_alpha = 0.0f;
        smo.e_beta = 0.0f;
        smo.e_alpha_filtered = 0.0f;
        smo.e_beta_filtered = 0.0f;

        smo.alignment_time++;

        // 对齐时间：100ms（2000次采样）
        if (smo.alignment_time >= 2000)
        {
            smo.is_aligned = true;
            smo.startup_gain = 0.0f; // 从0开始逐渐增加观测器增益
        }

        // 角度和速度保持0或开环估计
        smo.theta = 0.0f; // 或者使用开环角度
        smo.omega = 0.0f;
        smo.theta_prev = smo.theta;
        return;
    }

    // ✅ 阶段2：渐进式启动（增益从0逐渐增加）
    smo.startup_gain += 0.001f; // 每50μs增加0.001，约50ms达到满增益
    smo.startup_gain = clampf(smo.startup_gain, 0.0f, 1.0f);

    float current_gain = smo.startup_gain;
    float observer_gain = current_gain * (1.0f / (smo.Ls + smo.Rs * smo.dt));

    // ✅ 阶段3：防饱和积分器（核心改进）

    // 计算电流误差
    float i_alpha_error = i_alpha - smo.i_alpha_hat;
    float i_beta_error = i_beta - smo.i_beta_hat;

    // 电流误差限幅（防止大误差导致饱和）
    i_alpha_error = clampf(i_alpha_error, -2.0f, 2.0f); // ±2A限幅
    i_beta_error = clampf(i_beta_error, -2.0f, 2.0f);

    // ✅ 关键：使用PI型观测器，而不是纯积分器
    static float integrator_alpha = 0.0f;
    static float integrator_beta = 0.0f;

    // 比例项（主要项）
    float prop_alpha = observer_gain * (v_alpha - smo.Rs * smo.i_alpha_hat);
    float prop_beta = observer_gain * (v_beta - smo.Rs * smo.i_beta_hat);

    // 积分项（辅助项，需要严格限幅）
    integrator_alpha += observer_gain * (-smo.e_alpha) * smo.dt;
    integrator_beta += observer_gain * (-smo.e_beta) * smo.dt;

    // 积分器限幅
    integrator_alpha = clampf(integrator_alpha, -INTEGRATOR_LIMIT, INTEGRATOR_LIMIT);
    integrator_beta = clampf(integrator_beta, -INTEGRATOR_LIMIT, INTEGRATOR_LIMIT);

    // 估算电流更新
    float di_alpha_hat = (prop_alpha + integrator_alpha) * smo.dt;
    float di_beta_hat = (prop_beta + integrator_beta) * smo.dt;

    smo.i_alpha_hat += di_alpha_hat;
    smo.i_beta_hat += di_beta_hat;

    // 严格限幅估算电流
    smo.i_alpha_hat = clampf(smo.i_alpha_hat, -MAX_CURRENT_EST, MAX_CURRENT_EST);
    smo.i_beta_hat = clampf(smo.i_beta_hat, -MAX_CURRENT_EST, MAX_CURRENT_EST);

    // ✅ 阶段4：改进的滑模控制
    float normalized_alpha_error = i_alpha_error / (MAX_CURRENT_EST + 1e-6);
    float normalized_beta_error = i_beta_error / (MAX_CURRENT_EST + 1e-6);

    // 使用sigmoid函数平滑切换
    float sign_alpha = tanhf(normalized_alpha_error / smo.delta);
    float sign_beta = tanhf(normalized_beta_error / smo.delta);

    // 反电动势估算
    smo.e_alpha = current_gain * smo.k_sl * sign_alpha;
    smo.e_beta = current_gain * smo.k_sl * sign_beta;

    // 限幅反电动势
    smo.e_alpha = clampf(smo.e_alpha, -10.0f, 10.0f);
    smo.e_beta = clampf(smo.e_beta, -10.0f, 10.0f);

    // ✅ 阶段5：鲁棒滤波器
    float fc = 100.0f * current_gain + 50.0f; // 从50Hz逐渐增加到150Hz
    float alpha = 1.0f - expf(-MATH_2PI * fc * smo.dt);
    alpha = clampf(alpha, 0.01f, 0.99f);

    smo.e_alpha_filtered = (1.0f - alpha) * smo.e_alpha_filtered + alpha * smo.e_alpha;
    smo.e_beta_filtered = (1.0f - alpha) * smo.e_beta_filtered + alpha * smo.e_beta;

    // ✅ 阶段6：角度和速度计算（保持您已修正的版本）
    if (FABSF(smo.e_alpha_filtered) < 0.1f && FABSF(smo.e_beta_filtered) < 0.1f)
    {
        // 低速时使用开环估计
        static float open_loop_omega = 0.0f;
        open_loop_omega = 0.95f * open_loop_omega + 0.05f * smo.omega;
        smo.theta += open_loop_omega * smo.dt;
    }
    else
    {
        float theta_new = atan2f(smo.e_beta_filtered, smo.e_alpha_filtered);
        float diff = theta_new - smo.theta;
        if (diff > MATH_PI)
            diff -= MATH_2PI;
        if (diff < -MATH_PI)
            diff += MATH_2PI;
        smo.theta += 0.5f * diff; // 50%融合
    }

    smo.theta = fNormalizeAngle_0_2pi(smo.theta);

    float angle_diff = normalize_angle_pi_pi(smo.theta - smo.theta_prev);
    float speed_raw = angle_diff / smo.dt;
    speed_raw = clampf(speed_raw, -smo.max_omega, smo.max_omega);

    float speed_alpha = 0.1f + 0.4f * current_gain; // 从0.1逐渐增加到0.5
    smo.omega = (1.0f - speed_alpha) * smo.omega + speed_alpha * speed_raw;

    smo.theta_prev = smo.theta;
}

float fSMO_GetTheta()
{
    return smo.theta;
}

float fSMO_GetOmega()
{
    return smo.omega / smo.pole_pairs;
}
