#include "smo.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"

tSMO smo;

#define MAX_CURRENT_EST (MAX_Current / 2.0f) // 根据电机额定电流设置
#define INTEGRATOR_LIMIT 1000.0f             // 积分器限幅 (A/s)

/* 无感 SMO 观测器 */
void fSMO_Init(tMotor motor)
{
    smo.Rs = motor.Rs;
    smo.Ld = motor.Ld;
    smo.Lq = motor.Lq;
    smo.Psi_f = motor.Psi_f;
    smo.Ke = motor.Ke;
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
    smo.max_omega = 500.0f; // 最大电角速度 (rad/s)

    // 积分器保护
    smo.integrator_limit = 1000.0f; // A/s
}

void fSMO_Reset(void)
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

void fSMO_MainLoop(float v_alpha, float v_beta,
                   float i_alpha, float i_beta)
{
    // 阶段 1：启动对齐（最关键）
    if (!smo.is_aligned)
    {
        // 启动阶段：强制估算电流跟随实际电流，避免初始误差发散
        smo.i_alpha_hat = i_alpha;
        smo.i_beta_hat = i_beta;
        smo.e_alpha = 0.0f;
        smo.e_beta = 0.0f;
        smo.e_alpha_filtered = 0.0f;
        smo.e_beta_filtered = 0.0f;

        smo.alignment_time++;

        // 对齐时间：100ms（2000 次采样@20kHz）
        if (smo.alignment_time >= 2000)
        {
            smo.is_aligned = true;
            smo.startup_gain = 0.0f; // 从 0 开始逐渐增加观测器增益
        }

        // 角度和速度保持 0 或开环估计
        smo.theta = 0.0f;
        smo.omega = 0.0f;
        smo.theta_prev = smo.theta;
        return;
    }

    // 阶段 2：渐进式启动（增益从 0 逐渐增加）
    smo.startup_gain += 0.001f; // 每 50us 增加 0.001，约 50ms 达到满增益
    smo.startup_gain = CLAMP(smo.startup_gain, 0.0f, 1.0f);

    float current_gain = smo.startup_gain;

    // 观测器增益计算：使用 Ld/Lq 平均值，兼容表贴/内嵌电机
    float L_avg = (smo.Ld + smo.Lq) * 0.5f;
    float observer_gain = current_gain * (1.0f / (L_avg + smo.Rs * smo.dt));

    // 阶段 3：防饱和积分器（核心改进）

    // 计算电流误差
    float i_alpha_error = i_alpha - smo.i_alpha_hat;
    float i_beta_error = i_beta - smo.i_beta_hat;

    // 电流误差限幅（防止大误差导致饱和）
    i_alpha_error = CLAMP(i_alpha_error, -2.0f, 2.0f); // ±2A 限幅
    i_beta_error = CLAMP(i_beta_error, -2.0f, 2.0f);

    // 关键：使用 PI 型观测器，而不是纯积分器
    static float integrator_alpha = 0.0f;
    static float integrator_beta = 0.0f;

    // 比例项（主要项）
    float prop_alpha = observer_gain * (v_alpha - smo.Rs * smo.i_alpha_hat);
    float prop_beta = observer_gain * (v_beta - smo.Rs * smo.i_beta_hat);

    // 积分项（辅助项，需要严格限幅）
    integrator_alpha += observer_gain * (-smo.e_alpha) * smo.dt;
    integrator_beta += observer_gain * (-smo.e_beta) * smo.dt;

    // 积分器限幅
    integrator_alpha = CLAMP(integrator_alpha, -INTEGRATOR_LIMIT, INTEGRATOR_LIMIT);
    integrator_beta = CLAMP(integrator_beta, -INTEGRATOR_LIMIT, INTEGRATOR_LIMIT);

    // 估算电流更新
    float di_alpha_hat = (prop_alpha + integrator_alpha) * smo.dt;
    float di_beta_hat = (prop_beta + integrator_beta) * smo.dt;

    smo.i_alpha_hat += di_alpha_hat;
    smo.i_beta_hat += di_beta_hat;

    // 严格限幅估算电流
    smo.i_alpha_hat = CLAMP(smo.i_alpha_hat, -MAX_CURRENT_EST, MAX_CURRENT_EST);
    smo.i_beta_hat = CLAMP(smo.i_beta_hat, -MAX_CURRENT_EST, MAX_CURRENT_EST);

    // 阶段 4：改进的滑模控制
    float normalized_alpha_error = i_alpha_error / (MAX_CURRENT_EST + 1e-6f);
    float normalized_beta_error = i_beta_error / (MAX_CURRENT_EST + 1e-6f);

    // 使用 sigmoid 函数平滑切换，减少抖振
    float sign_alpha = tanhf(normalized_alpha_error / smo.delta);
    float sign_beta = tanhf(normalized_beta_error / smo.delta);

    // 反电动势估算
    smo.e_alpha = current_gain * smo.k_sl * sign_alpha;
    smo.e_beta = current_gain * smo.k_sl * sign_beta;

    // 限幅反电动势
    smo.e_alpha = CLAMP(smo.e_alpha, -10.0f, 10.0f);
    smo.e_beta = CLAMP(smo.e_beta, -10.0f, 10.0f);

    // 阶段 5：鲁棒滤波器
    float fc = 100.0f * current_gain + 50.0f; // 从 50Hz 逐渐增加到 150Hz
    float alpha = 1.0f - expf(-MATH_2PI * fc * smo.dt);
    alpha = CLAMP(alpha, 0.01f, 0.99f);

    smo.e_alpha_filtered = (1.0f - alpha) * smo.e_alpha_filtered + alpha * smo.e_alpha;
    smo.e_beta_filtered = (1.0f - alpha) * smo.e_beta_filtered + alpha * smo.e_beta;

    // 阶段 6：角度和速度计算
    if (FABSF(smo.e_alpha_filtered) < 0.1f && FABSF(smo.e_beta_filtered) < 0.1f)
    {
        // 低速时反电动势太小，使用开环估计避免角度跳变
        static float open_loop_omega = 0.0f;
        open_loop_omega = 0.95f * open_loop_omega + 0.05f * smo.omega;
        smo.theta += open_loop_omega * smo.dt;
    }
    else
    {
        // 正常速度：用反正切计算电角度
        float theta_new = atan2f(smo.e_beta_filtered, smo.e_alpha_filtered) * 57.29578f;
        float diff = theta_new - smo.theta;
        diff = fNormalizeAngle_180(diff);

        smo.theta += 0.5f * diff; // 50% 融合，平滑过渡
    }
    smo.theta = fNormalizeAngle_0_360(smo.theta);

    // 速度计算：角度差分 + 低通滤波
    float angle_diff = fNormalizeAngle_180(smo.theta - smo.theta_prev);
    float speed_raw = angle_diff / smo.dt;
    speed_raw = CLAMP(speed_raw, -smo.max_omega, smo.max_omega);

    float speed_alpha = 0.1f + 0.4f * current_gain; // 从 0.1 逐渐增加到 0.5
    smo.omega = (1.0f - speed_alpha) * smo.omega + speed_alpha * speed_raw;

    smo.theta_prev = smo.theta;
}

float fSMO_GetTheta(void)
{
    return smo.theta;
}

float fSMO_GetRPM(void)
{
    return smo.omega * 0.16666666666667f / (float)smo.pole_pairs;
}