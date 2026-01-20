#include "smo.h"
#include "math_fast.h"
#include "system_parameters.h"
#include "foc_core.h"
#include "encoder.h"
#include "parameter_manager.h"
smo_t smo;
smo_t *get_smo_adr()
{
    return &smo;
}
/*无感SMO观测器*/
void smo_init(float Rs, float Ls, float Psi_f, float max_speed, short WireS, float pole_pairs,
              float Ke, float J, float B)
{
    smo.Rs = Rs;
    smo.Ls = Ls;
    smo.Psi_f = Psi_f;
    smo.wire_sequence = WireS;
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
    // todo:返线序的处理
    //  计算速度（角度微分）- 改进：添加滤波和限幅
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
    return smo.omega / smo.pole_pairs;
}
void write_motor_param()
{
    g_Param.theta_offset = smo.theta_offset;
    g_Param.motor_rs = smo.Rs;
    g_Param.motor_ls = smo.Ls;
    g_Param.motor_psif = smo.Psi_f;
    g_Param.motor_ke = smo.Ke;
    g_Param.motor_j = smo.J;
    g_Param.motor_b = smo.B;
}
/*SMO整定器*/
// 一个周期50us 20 1ms 20000 1s
#define WS_delta_uq 0.5f      // uq 电压增量
#define WS_samples (u32)40000 // 3s 每个电压作用时间
#define WS_MAX_uq 5.0f        // 最大电压

#define THETA_OFFSET_samples (u32)20000                // 1s
#define THETA_OFFSET_timeout THETA_OFFSET_samples * 10 // 10s

#define RS_delta_v 0.5f // 电压增量
#define RS_samples (u32)20000
#define RS_timeout RS_samples * 10

#define Ls_inject_f (u8)1000
#define Ls_inject_u 0.8f // alpha beta 注入0.5v 5khz电压
#define Ls_tic (u16)(fpwm / Ls_inject_f)
#define Ls_samples (u32)60000     // 3s
#define Ls_timeout Ls_samples * 4 // 12s
#define MIN_di_dt 50

#define POLE_PAIRS_uq 0.0f
#define POLE_PAIRS_omega_elec 10.f                // 100rpm 电角速度
#define POLE_PAIRS_samples (u32)100000            // 5s
#define POLE_PAIRS_timeout POLE_PAIRS_samples * 3 // 15s

#define PK_omega_mech 5.f
#define PK_samples (u32)20000      // 1s
#define PK_timeout PK_samples * 10 // 10s

param_tuning_t tun = {0};
param_tuning_t *get_tuning_adr()
{
    return &tun;
}

void param_tuning_init(float udc)
{
    memset(&tun, 0, sizeof(tun) - 3);
    tun.dt = Tcon;
    tun.Udc = udc;
}

// 1.角度偏移整定

static bool param_tune_theta_offset(float theta_elec)
{
    // 稳态点采样
    if (theta_elec - tun.theta_elec_prev < 0.017f)
    {
        tun.tune_samples++; // 稳态计数
        if (tun.tune_samples > THETA_OFFSET_samples - 100)
        { // 最后100个求平均
            smo.theta_offset += theta_elec * 0.01f;
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
    tun.theta_elec_prev = theta_elec;
    // 完成
    if (tun.tune_samples >= THETA_OFFSET_samples)
    {
        SET_Theta_offset(smo.theta_offset);
        return true;
    }
    return false;
}
// 2. 电阻整定
static bool param_tune_Rs(float v_alpha, float i_alpha)
{

    static float prev_i_alpha = 0.0f;

    if (fabsf(i_alpha - prev_i_alpha) < 0.3f)
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
    else
    {
        tun.steady_i = 0.0f;
        tun.steady_v = 0.0f;
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
    static float prev_i = 0.0f;
    tun.tune_samples++;
    if (tun.alpha_beta_flag == false)
    { // alpha L 辨识
        if (v_alpha > Ls_inject_u * 0.8f)
        {
            float di_alpha = (i_alpha - prev_i) / tun.dt;
            // alpha轴：电流应上升
            if (di_alpha > MIN_di_dt)
            {
                tun.sum_di_dt_alpha_pos += di_alpha;
                tun.alpha_pos_count++;
            }
        }
        else if (v_alpha < -Ls_inject_u * 0.8f)
        {
            float di_alpha = (i_alpha - prev_i) / tun.dt;
            // alpha轴：电流应下降
            if (di_alpha < -MIN_di_dt)
            {
                tun.sum_di_dt_alpha_neg += di_alpha;
                tun.alpha_neg_count++;
            }
        }
        prev_i = i_alpha;
    }
    else
    { // beta L 辨识
        if (v_beta > Ls_inject_u * 0.8f)
        {
            float di_beta = (i_beta - prev_i) / tun.dt;
            // beta轴：电流应上升（同步注入，应有相似变化率）
            if (di_beta > MIN_di_dt)
            {
                tun.sum_di_dt_beta_pos += di_beta;
                tun.beta_pos_count++;
            }
        }
        // 负向方波阶段（两个轴同时为负）
        else if (v_beta < -Ls_inject_u * 0.8f)
        {
            float di_beta = (i_beta - prev_i) / tun.dt;

            // beta轴：电流应下降
            if (di_beta < -MIN_di_dt)
            {
                tun.sum_di_dt_beta_neg += di_beta;
                tun.beta_neg_count++;
            }
        }
        prev_i = i_beta;
    }
    // === 完成判断 ===
    if (tun.tune_samples >= Ls_samples)
    {
        if (tun.alpha_beta_flag == false)
        {
            tun.alpha_beta_flag = true;
            tun.tune_samples = 0;
            prev_i = 0.0f;
            return false;
        }
        // 验证数据有效性（两个轴都需要足够样本）
        bool alpha_valid = (tun.alpha_pos_count >= 20 && tun.alpha_neg_count >= 20);
        bool beta_valid = (tun.beta_pos_count >= 20 && tun.beta_neg_count >= 20);

        if (!alpha_valid && !beta_valid)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_LS;
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

        if (smo.Ls < 0.00001f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_LOW;
            tun.fault_state = PARAM_TUNE_LS;
            return true;
        }
        if (smo.Ls > 0.001f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_HIGH;
            tun.fault_state = PARAM_TUNE_LS;
            return true;
        }
				return true;
    }
    return false;
}

// 4. 极对数整定
static bool param_tune_pole_pairs(float omega_mech, u8 pole_pairs_input)
{
    static float omega_smo_last = 0;
    static float pole_pairs = 0;
    if ((omega_mech - tun.omega_mech_prev) < 0.1f && (smo.omega - omega_smo_last) < 1.f)
    {
        tun.tune_samples++;
    }
    else
        tun.tune_samples = 0;

    if (tun.tune_samples > POLE_PAIRS_samples - 100)
    {
        pole_pairs += (smo.omega / omega_mech) * 0.01f;
    }
    else
    {
        pole_pairs = 0.0f;
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
static bool param_tune_Psi_f(float omega_mech)
{
    float speed_electrical = omega_mech * smo.pole_pairs;

    float e_mag = sqrtf(smo.e_alpha_filtered * smo.e_alpha_filtered + smo.e_beta_filtered * smo.e_beta_filtered);

    if (fabsf(e_mag) > 0.5f && fabsf(speed_electrical) > 100.0f)
        tun.tune_samples++;
    else
        tun.tune_samples = 0;

    if (tun.tune_samples >= PK_samples - 100)
    {
        tun.sum_e_mag += e_mag;
        tun.sum_speed += fabsf(speed_electrical);
    }
    else
    {
        tun.sum_e_mag = 0.0f;
        tun.sum_speed = 0.0f;
    }
    if (tun.tune_samples >= PK_samples)
    {
        float avg_e_mag = tun.sum_e_mag / 100.0f;
        float avg_speed = tun.sum_speed / 100.0f;

        smo.Psi_f = avg_e_mag / avg_speed;
        smo.Ke = smo.Psi_f * smo.pole_pairs;
        if (smo.Psi_f > 10.0f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_HIGH;
            tun.fault_state = PARAM_TUNE_PK;
        }
        if (smo.Psi_f < 0.01f)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_LOW;
            tun.fault_state = PARAM_TUNE_PK;
        }
        return true;
    }
    return false;
}

// 6. J/B整定
static bool param_tune_JB(float omega_mech, float iq)
{
    float accel = (omega_mech - tun.omega_mech_prev) / tun.dt;
    // 从2rad/s开始采样
    if (fabsf(omega_mech) < 2.0f)
        return false;
    tun.tune_samples++;
    tun.sum_accel += accel;
    tun.sum_iq += iq;
    if (tun.tune_samples > 100 || fabsf(omega_mech - PK_omega_mech) < 1.f)
    {
        float avg_accel = tun.sum_accel / tun.tune_samples;
        float avg_torque = tun.sum_iq * smo.Ke / tun.tune_samples;
        if (fabsf(avg_accel) < 5.0f)
        {
            smo.J = 0.001f;
        }
        else
        {
            smo.J = avg_torque / avg_accel;
            if (smo.J < 0.0001f)
            {
                tun.fault_flag = true;
                tun.fault_type = PARAM_FAULT_LOW;
                tun.fault_state = PARAM_TUNE_JB;
            }
            if (smo.J > 0.01f)
            {
                tun.fault_flag = true;
                tun.fault_type = PARAM_FAULT_HIGH;
                tun.fault_state = PARAM_TUNE_JB;
            }
        }
        if (fabsf(accel) < 1.0f && fabsf(omega_mech) > 5.0f)
        {
            smo.B = (avg_torque - smo.J * 0.1f) / omega_mech;
            if (smo.B < 0.0001f)
                smo.B = 0.001f;
        }
        else
        {
            smo.B = 0.001f;
        }
        return true;
    }
    return false;
}

// 有感整定更新
static float omega_last = 0;
static float rel_theta_elec = 0;
void param_tuning_update(float *theta_elec, float theta_mech, float *u_alpha, float *u_beta,
                         float i_alpha, float i_beta, float omega_mech, u8 pole_pairs_input, float i_q)
{

    // 参数整定状态机
    switch (tun.tune_state)
    {
    case PARAM_TUNE_IDLE:
        // 线序整定的前置条件
        FOC_SET_RUNMODE(ENCODER_CONTROL);
        FOC_SET_LOOPMODE(VOLTAGE_LOOP);
        SET_Wire_sequence(1); // 设为正线序
        smo.wire_sequence = 1;
        tun.cur_uq_ud[0] = WS_delta_uq;
        tun.cur_uq_ud[1] = 0.0f;
        FOC_SET_VER_VALUE(tun.cur_uq_ud);
        tun.tune_state = PARAM_TUNE_WireS;
        break;
    case PARAM_TUNE_WireS:
        // 控制 先设置为正线序 施加uq=0.5 监测转动情况 如果可以转动 则为正线序 不能转动再设置为反线序号 施加uq=-0.5 监测转动情况 如果可以转动 则为反线序号 不能转动则为线序失效 然后再次设置为正线序 施加uq=0.5 监测转动情况 如果可以转动 则为正线序 不能转动则为线序失效 然后再次设置为反线序号 施加uq=-0.5 监测转动情况 如果可以转动 则为反线序号 不能转动则为线序失效 然后再次设置为正线序 施加uq=0.5 监测转动情况 如果可以转动 则为正线序 不能转动则为线序失效 然后再次设置为反线序号
        //  施加uq=0.5 监测转动情况 如果可以转动 则为反线序号
        // 不能转动 增加uq 知道转动 或者过电流
        if (fabsf(omega_mech - omega_last) > 0.5f)
        { // 非稳态
            omega_last = omega_mech;
            tun.tune_samples = 0;
            return;
        }
        // 条件判断
        if (fabsf(omega_mech) < 0.3f)
        {
            tun.steady_samples = 0;
            tun.tune_samples++;

            if (tun.tune_samples > WS_samples)
            {
                if (WS_delta_uq * (tun.num_test_wire + 1) > WS_MAX_uq)
                {
                    tun.fault_flag = true;
                    tun.fault_type = PARAM_FAULT_WS_LOCKED;
                    tun.fault_state = PARAM_TUNE_WireS;
                }
                if (smo.wire_sequence == 1)
                { // 正线序不转 尝试反线序
                    smo.wire_sequence = -1;
                    SET_Wire_sequence(-1); // 设为反线序
                }
                else
                { // 反线序也不转 改为正线序 增大uq
                    smo.wire_sequence = 1;
                    SET_Wire_sequence(1); // 设为正线序
                    tun.num_test_wire++;
                    tun.cur_uq_ud[0] = WS_delta_uq * (tun.num_test_wire + 1);
                    tun.cur_uq_ud[1] = 0;
                    FOC_SET_VER_VALUE(tun.cur_uq_ud);
                }
                tun.tune_samples = 0;
            }
        }
        else
        {
            tun.steady_samples++;
            if (tun.steady_samples > WS_samples / 2)
            {
                // 前置条件
                tun.cur_uq_ud[0] = 0;
                tun.cur_uq_ud[1] = 0.3f;
                FOC_SET_VER_VALUE(tun.cur_uq_ud);
                tun.tune_samples = 0;
                tun.time_tic = 0;
                SET_Theta_offset(0.0f);
                tun.tune_state = PARAM_TUNE_THETA_OFFSET;
            }
        }
        break;
    case PARAM_TUNE_THETA_OFFSET:
        // 控制
        rel_theta_elec = *theta_elec;
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
        if (param_tune_theta_offset(rel_theta_elec))
        { // 完成跳转
          // 前置条件
            FOC_SET_RUNMODE(SVPWM_CONTROL);
            *u_alpha = RS_delta_v;
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
        if (!tun.alpha_beta_flag)
        {
            *u_alpha = tun.inject_flag ? +Ls_inject_u : -Ls_inject_u;
            *u_beta = 0;
        }
        else
        {
            *u_beta = tun.inject_flag ? +Ls_inject_u : -Ls_inject_u;
            *u_alpha = 0;
        }
        // 不用超时监测
        //  稳态点采样
        if (param_tune_Ls(*u_alpha, *u_beta, i_alpha, i_beta))
        { // 完成跳转
            FOC_SET_LOOPMODE(VOLTAGE_LOOP);
            float cur_uq_ud[2] = {POLE_PAIRS_uq, 0};
            FOC_SET_VER_VALUE(cur_uq_ud);
            smo.pole_pairs = 1;
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_POLE_PAIRS; // 进入下一步
        }
        break;
    case PARAM_TUNE_POLE_PAIRS:
        // 控制
        smo_update(*u_alpha, *u_beta, i_alpha, i_beta);
        tun.theta_elec_con += tun.dt * POLE_PAIRS_omega_elec;
        tun.theta_elec_con = normalize_angle_0_2pi(tun.theta_elec_con);
        *theta_elec = smo.wire_sequence*tun.theta_elec_con;
        if (fabsf(theta_mech - tun.omega_mech_prev) > 1.f)
            return;
        // 超时监测
        tun.time_tic++;
        if (tun.time_tic > POLE_PAIRS_timeout)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_RS;
        }
        // 稳态点采样
        if (param_tune_pole_pairs(omega_mech, pole_pairs_input))
        { // 完成跳转 磁链整定 前置条件
            FOC_SET_LOOPMODE(SPEED_LOOP);
            tun.omega_ref = 10;
            FOC_SET_VER_VALUE(&tun.omega_ref);
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_POLE_PAIRS; // 进入下一步
        }
        break;
    case PARAM_TUNE_PK:
        // 控制
        tun.theta_elec_con = theta_mech * smo.pole_pairs;
        tun.theta_elec_con = normalize_angle_0_2pi(tun.theta_elec_con);
        *theta_elec = smo.wire_sequence*tun.theta_elec_con;
        smo_update(*u_alpha, *u_beta, i_alpha, i_beta);
        if (fabsf(theta_mech - tun.omega_mech_prev) > 1.f)
            return;
        // 超时监测
        tun.time_tic++;
        if (tun.time_tic > PK_timeout)
        {
            tun.fault_flag = true;
            tun.fault_type = PARAM_FAULT_TIMEOUT;
            tun.fault_state = PARAM_TUNE_RS;
        }

        // 稳态点采样
        if (param_tune_Psi_f(omega_mech))
        {
            // 完成跳转
            FOC_SET_LOOPMODE(SPEED_LOOP);
            tun.omega_ref = 0;
            FOC_SET_VER_VALUE(&tun.omega_ref);
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_POLE_PAIRS; // 进入下一步
        }
        break;
    case PARAM_TUNE_JB:
        // 等待速度降0
        if (!tun.start_smp_flag)
        {
            if (fabsf(omega_mech) < 0.1f)
                tun.start_smp_flag = true;
            return;
        }
        // 施加阶跃速度
        tun.omega_ref = 10;
        FOC_SET_VER_VALUE(&tun.omega_ref);
        FOC_SET_OMEGA_con(tun.omega_ref);

        // 直接开始采样
        if (param_tune_JB(omega_mech, i_q))
        {
            // 完成跳转
            tun.omega_ref = 0;
            FOC_SET_VER_VALUE(&tun.omega_ref);
            tun.tune_samples = 0;
            tun.time_tic = 0;
            tun.tune_state = PARAM_TUNE_COMPLETE; // 进入下一步
        }
        break;
    case PARAM_TUNE_COMPLETE:
        if (tun.fault_flag)
            return;
        write_motor_param();
        break;
    default:
        break;
    }
    tun.omega_mech_prev = omega_mech;
    if (tun.fault_flag)
        tun.tune_state = PARAM_TUNE_COMPLETE;
}
param_tune_state_t param_tuning_get_state()
{
    return tun.tune_state;
}