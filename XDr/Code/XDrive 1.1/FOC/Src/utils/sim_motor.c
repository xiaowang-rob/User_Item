#include "sim_motor.h"
#include "math_fast.h"
#include "system_parameters.h"
PMSM_Motor gMotor = {
    .params = {
        .Rs = 0.05f,     // 定子电阻
        .Ld = 0.0002f,   // d轴电感
        .Lq = 0.0002f,   // q轴电感 (表贴式电机Ld=Lq)
        .psi_f = 0.01f,  // 永磁体磁链
        .pole_pairs = 7, // 4对极
        .J = 0.001f,     // 转动惯量
        .B = 0.0005f,    // 阻尼系数
        .Tl = 0.3f,      // 负载转矩
        .dt = Tcon,      // 10μs仿真步长
    },
    .state = {
        .ia = 0.0f, // 初始三相电流
        .ib = 0.0f,
        .ic = 0.0f,
        .ialpha = 0.0f,
        .ibeta = 0.0f,
        .iq = 0.0f,
        .id = 0.0f,
        .ud = 0.0f,
        .uq = 0.0f,
        .pos_m = 2.0f,   // 初始转子位置
        .theta_m = 0.0f, // 初始转子位置
        .omega_m = 0.0f, // 初始转速
        .theta = 0.0f    // 初始电角度
    }};

const float STATIC_FRICTION_FACTOR = 1.5f;
const float ZERO_SPEED_THRESHOLD = 0.1f; // rad/s
// 仿真一步
void motor_step(float va, float vb, float vc)
{
    float did_dt, diq_dt;
    float Te;                                                        // 电磁转矩
    float omega_e = gMotor.params.pole_pairs * gMotor.state.omega_m; // 电角速度

    // 2. 将三相电压变换到dq坐标系
    clark_transform(va, vb, vc, &gMotor.state.ualpha, &gMotor.state.ubeta);
    park_transform(gMotor.state.ualpha, gMotor.state.ubeta, gMotor.state.theta, &gMotor.state.ud, &gMotor.state.uq);

    // 3. 计算电流变化率 (dq坐标系)
    did_dt = (gMotor.state.ud - gMotor.params.Rs * gMotor.state.id + omega_e * gMotor.params.Lq * gMotor.state.iq) / gMotor.params.Ld;

    diq_dt = (gMotor.state.uq - gMotor.params.Rs * gMotor.state.iq - omega_e * gMotor.params.Ld * gMotor.state.id - omega_e * gMotor.params.psi_f) / gMotor.params.Lq;

    // 4. 更新dq轴电流 (前向欧拉法)
    float id_new = gMotor.state.id + did_dt * gMotor.params.dt;
    float iq_new = gMotor.state.iq + diq_dt * gMotor.params.dt;
    gMotor.state.id = gMotor.state.id * 0.9f + id_new * 0.1f;
    gMotor.state.iq = gMotor.state.iq * 0.9f + iq_new * 0.1f;

    // 5. 计算电磁转矩

    Te = 1.5f * gMotor.params.pole_pairs *
         (gMotor.params.psi_f * gMotor.state.iq +
          (gMotor.params.Ld - gMotor.params.Lq) * gMotor.state.id * gMotor.state.iq);
    // 6. 计算机械角加速度
    float domega_dt;
    float abs_omega = fabs(gMotor.state.omega_m);

    // 情况1: 电机处于静止状态
    if (abs_omega < ZERO_SPEED_THRESHOLD)
    {
        float static_friction = STATIC_FRICTION_FACTOR * gMotor.params.B;
        float total_resistance = static_friction + fabs(gMotor.params.Tl);

        // 检查净转矩是否足以克服静摩擦
        if (fabs(Te) <= total_resistance)
        {
            // 转矩不足 → 保持静止
            domega_dt = 0.0f;
            gMotor.state.omega_m = 0.0f;
        }
        // 足够启动
        else
        {
            float direction = (Te > 0) ? 1.0f : -1.0f;
            float net_torque = Te - direction * (gMotor.params.Tl + static_friction);
            domega_dt = net_torque / gMotor.params.J;
        }
    }
    // 情况2: 电机正在旋转 (动摩擦)
    else
    {
        float dynamic_friction = gMotor.params.B * gMotor.state.omega_m;
        float net_torque = Te - dynamic_friction - gMotor.params.Tl;
        domega_dt = net_torque / gMotor.params.J;

        // 防止穿越零速时的不连续性
        float predicted_omega = gMotor.state.omega_m + domega_dt * gMotor.params.dt;
        if (fabs(predicted_omega) < ZERO_SPEED_THRESHOLD)
        {
            float static_friction = STATIC_FRICTION_FACTOR * gMotor.params.B;
            float total_resistance = static_friction + fabs(gMotor.params.Tl);

            if (fabs(Te) < total_resistance)
            {
                // 预计会停在零速
                gMotor.state.omega_m += domega_dt * gMotor.params.dt;
                if (fabs(gMotor.state.omega_m) < ZERO_SPEED_THRESHOLD)
                {
                    gMotor.state.omega_m = 0.0f;
                }
                domega_dt = 0.0f; // 下一步将保持静止
            }
        }
    }

    // 8. 更新转子位置
    gMotor.state.pos_m += gMotor.state.omega_m * gMotor.params.dt;
    gMotor.state.omega_m += domega_dt * gMotor.params.dt;
    gMotor.state.theta_m = normalize_angle_0_2pi(gMotor.state.pos_m);

    gMotor.state.theta = gMotor.state.theta_m * gMotor.params.pole_pairs;
    // 9. 角度归一化到 [0, 2π)
    gMotor.state.theta = normalize_angle_0_2pi(gMotor.state.theta);

    // 10. 将更新后的dq电流变换回三相线电流
    inv_park_transform(gMotor.state.id, gMotor.state.iq, gMotor.state.theta, &gMotor.state.ialpha, &gMotor.state.ibeta);
    inv_clark_transform(gMotor.state.ialpha, gMotor.state.ibeta, &gMotor.state.ia, &gMotor.state.ib, &gMotor.state.ic);
}
