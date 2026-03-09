#include "tune.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"

/* ================================= 全局变量定义 ================================= */
tMotorParams g_motor_params = {0};
tTuneContext g_tune_ctx = {0};

/* ================================= 默认参数 ================================= */
#define DEFAULT_RS 0.08f
#define DEFAULT_LS 80e-6f
#define DEFAULT_POLE_PAIRS 7
#define DEFAULT_KV 550.0f
#define DEFAULT_UDC 25.0f
#define DEFAULT_DT 0.00005f

/* ================================= 整定参数配置 ================================= */
// 通用时间转换 (假设 20kHz 中断，1 tick = 50us)
#define TICK_TO_MS(tick) ((tick) * 0.05f)
#define MS_TO_TICK(ms) ((u16)((ms) * 20.0f))

// 电阻整定
#define RS_I_MIN 2.0f
#define RS_I_MAX 10.0f
#define RS_V_LIMIT 2.0f
#define RS_ERR_TOL 0.1f
#define RS_MIN_DELTA_I 0.5f
#define RS_RANGE_MIN 0.02f
#define RS_RANGE_MAX 0.5f
#define RS_STEADY_TICKS 200

// 电感整定
#define LS_V_START 0.2f
#define LS_V_MAX 1.0f
#define LS_I_LIMIT 5.0f
#define LS_MIN_DI_DT 50.0f
#define LS_MAX_DI_DT 5000.0f
#define LS_RANGE_MIN 20e-6f
#define LS_RANGE_MAX 300e-6f

// 角度偏移
#define THETA_ID_INJ 2.0f
#define THETA_IQ_MAX 0.3f
#define THETA_OMEGA_MAX 0.05f
#define THETA_STEADY_WIN 100
#define THETA_VERIFY_CYCLES 3
#define THETA_MAX_DIFF 0.05f

// 线序
#define WS_VF_FREQ 3.0f
#define WS_VF_AMP 1.5f
#define WS_DURATION_TICKS MS_TO_TICK(300)
#define WS_WAIT_STOP_TICKS MS_TO_TICK(300)
#define WS_ENC_DELTA_MIN 50

// 极对数
#define POLE_OMEGA_REF 200.0f  // 测试电角速度 200 rad/s
#define POLE_STEADY_TICKS 1000 // 稳态采样 50ms
#define POLE_RATIO_MIN 0.5f
#define POLE_RATIO_MAX 16.0f

// 磁链
#define PSI_MIN_OMEGA 100.0f // 最小测试速度
#define PSI_MIN_E_MAG 0.5f   // 最小反电动势幅值
#define PSI_STEADY_TICKS 500 // 采样 25ms

// 机械参数
#define JB_ACCEL_TIME_TICKS MS_TO_TICK(500) // 加速时间 500ms
#define JB_MIN_OMEGA 2.0f                   // 最小采样速度
#define JB_MIN_ACCEL 5.0f                   // 最小有效加速度

/* ================================= 参数访问实现 ================================= */

bool fMotorParam_Validate(const tMotorParams *params)
{
    if (!params)
        return false;
    if (params->Rs < RS_RANGE_MIN || params->Rs > RS_RANGE_MAX)
        return false;
    if (params->Ld < LS_RANGE_MIN || params->Ld > LS_RANGE_MAX)
        return false;
    if (params->Lq < LS_RANGE_MIN || params->Lq > LS_RANGE_MAX)
        return false;
    if (params->pole_pairs < 1 || params->pole_pairs > 16)
        return false;
    if (params->wire_sequence != 1 && params->wire_sequence != -1)
        return false;
    if (params->Ke > 0)
    {
        float Ke_calc = 60.0f / (2.0f * MATH_PI * DEFAULT_KV * params->pole_pairs);
        if (FABSF(params->Ke - Ke_calc) > 0.5f * Ke_calc)
            return false;
    }
    return true;
}
void fMotorParamTune_ForceSave(void)
{
    // TODO: 保存参数
    g_Param.motor_rs = g_motor_params.Rs;
    g_motor_params.Rs_valid = g_motor_params.L_valid = g_motor_params.offset_valid = true;
    g_motor_params.wire_valid = g_motor_params.pole_valid = g_motor_params.psi_valid = g_motor_params.mech_valid = true;
}
/* ================================= 初始化与重置 ================================= */
void fMotorParamTune_Init(const tMotorParams *params)
{
    fMotorParamTune_Reset();
    if (params)
    {
        g_motor_params = *params;
    }
    else
    {
        g_motor_params.Rs = DEFAULT_RS;
        g_motor_params.Ld = DEFAULT_LS;
        g_motor_params.Lq = DEFAULT_LS;
        g_motor_params.pole_pairs = DEFAULT_POLE_PAIRS;
        g_motor_params.Udc = DEFAULT_UDC;
        g_motor_params.dt = DEFAULT_DT;
        g_motor_params.wire_sequence = 1;
    }
    g_motor_params.Rs_valid = false;
    g_motor_params.L_valid = false;
    g_motor_params.offset_valid = false;
    g_motor_params.wire_valid = false;
    g_motor_params.pole_valid = false;
    g_motor_params.psi_valid = false;
    g_motor_params.mech_valid = false;
}

void fMotorParamTune_Reset(void)
{
    g_tune_ctx.state = TUNE_STATE_IDLE;
    g_tune_ctx.fault = TUNE_FAULT_NONE;
    g_tune_ctx.tick_count = 0;
    g_tune_ctx.timeout_tick = 0;

    tTuneContext *ctx = &g_tune_ctx;
    ctx->rs_ctx.step = 0;
    ctx->rs_ctx.target_i[0] = RS_I_MIN;
    ctx->rs_ctx.target_i[1] = RS_I_MIN * 2.5f;
    ctx->ls_ctx.v_inj = LS_V_START;
    ctx->ls_ctx.axis = false;
    ctx->ls_ctx.inject_cnt = 0;
    ctx->theta_ctx.valid_cnt = 0;
    ctx->theta_ctx.steady_flag = false;
    ctx->theta_ctx.hist_idx = 0;
    ctx->wire_ctx.step = 0;
    ctx->wire_ctx.enc_delta_fwd = 0;
    ctx->wire_ctx.enc_delta_rev = 0;
    ctx->pole_ctx.omega_ref = POLE_OMEGA_REF;
    ctx->pole_ctx.valid_cnt = 0;
    ctx->psi_ctx.valid_cnt = 0;
    ctx->psi_ctx.ready = false;
    ctx->jb_ctx.sample_cnt = 0;
    ctx->jb_ctx.ready = false;
    for (int i = 0; i < 4; i++)
    {
        ctx->temp_val[i] = 0;
        ctx->temp_cnt[i] = 0;
        ctx->temp_flag[i] = false;
    }
}

/* ================================= 电阻整定 ================================= */
static bool _tune_Rs(float v_alpha, float i_alpha, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    float target_i = ctx->rs_ctx.target_i[ctx->rs_ctx.step];
    float i_err = target_i - i_alpha;
    float v_cmd = i_err * 0.3f;
    v_cmd = CLAMP(v_cmd, -RS_V_LIMIT, RS_V_LIMIT);

    // TODO: 施加电压
    // fFOC_SetUalphaBeta(v_cmd, 0);
    ctx->rs_ctx.v_cmd_last = v_cmd;

    static float i_last = 0;
    if (FABSF(i_alpha - i_last) < 0.05f && FABSF(i_err) < RS_ERR_TOL)
        ctx->tick_count++;
    else
        ctx->tick_count = 0;
    i_last = i_alpha;

    if (FABSF(v_cmd) >= RS_V_LIMIT * 0.9f && ctx->tick_count == 0)
    {
        ctx->rs_ctx.target_i[ctx->rs_ctx.step] *= 0.8f;
        if (ctx->rs_ctx.target_i[ctx->rs_ctx.step] < RS_I_MIN)
        {
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            return true;
        }
        ctx->tick_count = 0;
    }

    if (ctx->tick_count >= RS_STEADY_TICKS)
    {
        if (ctx->rs_ctx.step == 0)
        {
            ctx->rs_ctx.v_meas[0] = v_alpha;
            ctx->rs_ctx.i_meas[0] = i_alpha;
            ctx->rs_ctx.step = 1;
            ctx->rs_ctx.target_i[1] = CLAMP(target_i * 2.5f, RS_I_MIN, RS_I_MAX);
            ctx->tick_count = 0;
        }
        else if (ctx->rs_ctx.step == 1)
        {
            ctx->rs_ctx.v_meas[1] = v_alpha;
            ctx->rs_ctx.i_meas[1] = i_alpha;
            float delta_v = ctx->rs_ctx.v_meas[1] - ctx->rs_ctx.v_meas[0];
            float delta_i = ctx->rs_ctx.i_meas[1] - ctx->rs_ctx.i_meas[0];
            if (FABSF(delta_i) < RS_MIN_DELTA_I)
            {
                ctx->fault = TUNE_FAULT_SIGNAL_WEAK;
                return true;
            }
            params->Rs = delta_v / (delta_i + 1e-6f);
            if (params->Rs < RS_RANGE_MIN || params->Rs > RS_RANGE_MAX)
            {
                ctx->fault = TUNE_FAULT_PARAM_INVALID;
                return true;
            }
            params->Rs_valid = true;
            return true;
        }
    }
    return false;
}

/* ================================= 电感整定 ================================= */
static bool _tune_Ls(float v_alpha, float v_beta, float i_alpha, float i_beta, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    if (ctx->tick_count % 100 == 0)
    {
        float i_meas = ctx->ls_ctx.axis ? i_beta : i_alpha;
        if (FABSF(i_meas) < LS_I_LIMIT * 0.3f && ctx->ls_ctx.v_inj < LS_V_MAX)
            ctx->ls_ctx.v_inj = CLAMP(ctx->ls_ctx.v_inj * 1.2f, LS_V_START, LS_V_MAX);
        else if (FABSF(i_meas) > LS_I_LIMIT * 0.9f)
            ctx->ls_ctx.v_inj *= 0.8f;
    }

    float tau = (params->Ld > 1e-6f) ? (params->Ld / (params->Rs + 0.01f)) : 0.001f;
    ctx->ls_ctx.inject_period = (u16)CLAMP(tau / params->dt * 8.0f, 20.0f, 200.0f);
    if (++ctx->ls_ctx.inject_cnt >= ctx->ls_ctx.inject_period)
    {
        ctx->ls_ctx.inject_cnt = 0;
        ctx->temp_flag[0] = !ctx->temp_flag[0];
    }

    float v_cmd = ctx->temp_flag[0] ? ctx->ls_ctx.v_inj : -ctx->ls_ctx.v_inj;
    float i_meas = ctx->ls_ctx.axis ? i_beta : i_alpha;
    if (FABSF(i_meas) > LS_I_LIMIT)
        v_cmd *= 0.5f;

    // TODO: 轴独立电压输出
    // if (!ctx->ls_ctx.axis) fFOC_SetUalphaBeta(v_cmd, 0);
    // else fFOC_SetUalphaBeta(0, v_cmd);

    float *i_prev = ctx->ls_ctx.axis ? &ctx->temp_val[2] : &ctx->temp_val[0];
    float di = i_meas - *i_prev;
    float di_dt = di / params->dt;
    *i_prev = i_meas;

    u8 axis = ctx->ls_ctx.axis ? 1 : 0;
    u8 half = ctx->temp_flag[0] ? 0 : 1;
    if (FABSF(v_cmd) > ctx->ls_ctx.v_inj * 0.8f && FABSF(di_dt) > LS_MIN_DI_DT && FABSF(di_dt) < LS_MAX_DI_DT)
    {
        ctx->ls_ctx.di_dt_sum[axis][half] += di_dt;
        ctx->ls_ctx.cnt[axis][half]++;
        if (FABSF(i_meas) > ctx->ls_ctx.i_peak[axis][half])
            ctx->ls_ctx.i_peak[axis][half] = FABSF(i_meas);
    }

    if (ctx->tick_count >= 2000)
    {
        if (!ctx->ls_ctx.axis)
        {
            ctx->ls_ctx.axis = true;
            ctx->tick_count = 0;
            ctx->ls_ctx.di_dt_sum[1][0] = ctx->ls_ctx.di_dt_sum[1][1] = 0;
            ctx->ls_ctx.cnt[1][0] = ctx->ls_ctx.cnt[1][1] = 0;
            ctx->ls_ctx.i_peak[1][0] = ctx->ls_ctx.i_peak[1][1] = 0;
            return false;
        }
        float L_alpha = 0, L_beta = 0;
        bool alpha_ok = (ctx->ls_ctx.cnt[0][0] >= 20 && ctx->ls_ctx.cnt[0][1] >= 20);
        bool beta_ok = (ctx->ls_ctx.cnt[1][0] >= 20 && ctx->ls_ctx.cnt[1][1] >= 20);
        if (alpha_ok)
        {
            float avg_di_dt = (FABSF(ctx->ls_ctx.di_dt_sum[0][0] / ctx->ls_ctx.cnt[0][0]) + FABSF(ctx->ls_ctx.di_dt_sum[0][1] / ctx->ls_ctx.cnt[0][1])) * 0.5f;
            float i_avg = (ctx->ls_ctx.i_peak[0][0] + ctx->ls_ctx.i_peak[0][1]) * 0.25f;
            float v_comp = ctx->ls_ctx.v_inj - params->Rs * i_avg;
            L_alpha = v_comp / (avg_di_dt + 1e-6f);
        }
        if (beta_ok)
        {
            float avg_di_dt = (FABSF(ctx->ls_ctx.di_dt_sum[1][0] / ctx->ls_ctx.cnt[1][0]) + FABSF(ctx->ls_ctx.di_dt_sum[1][1] / ctx->ls_ctx.cnt[1][1])) * 0.5f;
            float i_avg = (ctx->ls_ctx.i_peak[1][0] + ctx->ls_ctx.i_peak[1][1]) * 0.25f;
            float v_comp = ctx->ls_ctx.v_inj - params->Rs * i_avg;
            L_beta = v_comp / (avg_di_dt + 1e-6f);
        }
        if (alpha_ok && beta_ok)
        {
            params->Ld = params->Lq = (L_alpha + L_beta) * 0.5f;
        }
        else if (alpha_ok)
            params->Ld = params->Lq = L_alpha;
        else if (beta_ok)
            params->Ld = params->Lq = L_beta;
        else
        {
            ctx->fault = TUNE_FAULT_TIMEOUT;
            return true;
        }
        if (params->Ld < LS_RANGE_MIN || params->Ld > LS_RANGE_MAX)
        {
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            return true;
        }
        params->L_valid = true;
        return true;
    }
    return false;
}

/* ================================= 角度偏移整定 ================================= */
static bool _tune_ThetaOffset(float theta_elec, float omega_mech, float id_fb, float iq_fb, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    // TODO: 施加 id 电流锁轴
    // fFOC_SetTargetIdIq(THETA_ID_INJ, 0);

    bool omega_ok = FABSF(omega_mech) < THETA_OMEGA_MAX;
    bool i_ok = FABSF(id_fb - THETA_ID_INJ) < 0.1f && FABSF(iq_fb) < THETA_IQ_MAX;
    if (omega_ok && i_ok)
    {
        ctx->theta_ctx.valid_cnt++;
        if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN)
        {
            ctx->theta_ctx.theta_sum += theta_elec * 0.01f;
            if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 2)
                ctx->theta_ctx.steady_flag = true;
        }
    }
    else
    {
        if (ctx->theta_ctx.valid_cnt > 10)
            ctx->theta_ctx.valid_cnt -= 10;
        else
            ctx->theta_ctx.valid_cnt = 0;
        ctx->theta_ctx.theta_sum = 0;
    }

    if (ctx->theta_ctx.steady_flag && ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 3)
    {
        float offset = ctx->theta_ctx.theta_sum / (THETA_STEADY_WIN * 3 * 0.01f);
        offset = fNormalizeAngle_pi_pi(offset);
        ctx->theta_ctx.offset_hist[ctx->theta_ctx.hist_idx % THETA_VERIFY_CYCLES] = offset;
        ctx->theta_ctx.hist_idx++;
        if (ctx->theta_ctx.hist_idx >= THETA_VERIFY_CYCLES)
        {
            float max_diff = 0;
            for (u8 i = 0; i < THETA_VERIFY_CYCLES; i++)
                for (u8 j = i + 1; j < THETA_VERIFY_CYCLES; j++)
                {
                    float diff = FABSF(ctx->theta_ctx.offset_hist[i] - ctx->theta_ctx.offset_hist[j]);
                    if (diff > MATH_PI)
                        diff = MATH_2PI - diff;
                    if (diff > max_diff)
                        max_diff = diff;
                }
            if (max_diff < THETA_MAX_DIFF)
            {
                params->theta_offset = ctx->theta_ctx.offset_hist[0];
                params->offset_valid = true;
                return true;
            }
        }
    }
    return false;
}

/* ================================= 线序整定 ================================= */
static bool _tune_WireSequence(s32 *enc_delta_out, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    switch (ctx->wire_ctx.step)
    {
    case 0:
        // TODO: 关闭 PWM+ 重置编码器
        // fFOC_Disable_PWM(); Encoder_Reset();
        ctx->wire_ctx.step = 1;
        ctx->wire_ctx.step_start_tick = g_tune_ctx.tick_count;
        break;
    case 1:
        if (_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick) < WS_DURATION_TICKS)
        {
            float elapsed_ms = TICK_TO_MS(_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick));
            float angle = 2.0f * MATH_PI * WS_VF_FREQ * elapsed_ms * 0.001f;
            float Valpha = WS_VF_AMP * cosf(angle);
            float Vbeta = WS_VF_AMP * sinf(angle);
            // TODO: 开环电压输出
            // fFOC_SetUalphaBeta_OpenLoop(Valpha, Vbeta);
        }
        else
        {
            ctx->wire_ctx.step = 2;
            ctx->wire_ctx.step_start_tick = g_tune_ctx.tick_count;
            ctx->wire_ctx.wait_ticks = WS_WAIT_STOP_TICKS;
        }
        break;
    case 2:
        if (_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick) < ctx->wire_ctx.wait_ticks)
            break;
        // TODO: 读取编码器变化
        // ctx->wire_ctx.enc_delta_fwd = Encoder_Get_Delta();
        ctx->wire_ctx.step = 3;
        break;
    case 3:
        // TODO: 读取反向编码器变化
        // ctx->wire_ctx.enc_delta_rev = Encoder_Get_Delta();
        if (ctx->wire_ctx.enc_delta_fwd > WS_ENC_DELTA_MIN && ctx->wire_ctx.enc_delta_rev < -WS_ENC_DELTA_MIN)
        {
            params->wire_sequence = 1;
            ctx->wire_ctx.step = 4;
        }
        else if (ctx->wire_ctx.enc_delta_fwd < -WS_ENC_DELTA_MIN && ctx->wire_ctx.enc_delta_rev > WS_ENC_DELTA_MIN)
        {
            params->wire_sequence = -1;
            ctx->wire_ctx.step = 4;
        }
        else
        {
            ctx->fault = TUNE_FAULT_MECH_LOCKED;
            return true;
        }
        break;
    case 4:
        if (params->wire_sequence == -1)
        { /* TODO: 软件交换相序 */
        }
        params->wire_valid = true;
        if (enc_delta_out)
            *enc_delta_out = ctx->wire_ctx.enc_delta_fwd;
        return true;
    }
    return false;
}

/* ================================= 极对数整定 ================================= */
static bool _tune_PolePairs(float omega_mech, float omega_elec, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    // TODO: 确保电机以恒定速度运行 (速度环给定)
    // fFOC_SetSpeedRef(ctx->pole_ctx.omega_ref / params->pole_pairs);

    // 稳态检测
    if (FABSF(omega_mech - ctx->temp_val[0]) < 0.1f && FABSF(omega_elec - ctx->pole_ctx.omega_ref) < 5.0f)
    {
        ctx->pole_ctx.valid_cnt++;
        if (ctx->pole_ctx.valid_cnt >= POLE_STEADY_TICKS)
        {
            ctx->pole_ctx.steady_flag = true;
            ctx->pole_ctx.sum_ratio += (omega_elec / (omega_mech + 1e-6f));
        }
    }
    else
    {
        ctx->pole_ctx.valid_cnt = 0;
        ctx->pole_ctx.steady_flag = false;
    }
    ctx->temp_val[0] = omega_mech;

    if (ctx->pole_ctx.steady_flag && ctx->pole_ctx.valid_cnt >= POLE_STEADY_TICKS * 2)
    {
        float ratio = ctx->pole_ctx.sum_ratio / (ctx->pole_ctx.valid_cnt + 1e-6f);
        u8 pole_calc = (u8)(ratio + 0.5f);
        if (pole_calc < POLE_RATIO_MIN || pole_calc > POLE_RATIO_MAX)
        {
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            return true;
        }
        params->pole_pairs = pole_calc;
        params->pole_valid = true;
        return true;
    }
    return false;
}

/* ================================= 磁链整定 ================================= */
static bool _tune_PsiF(float omega_mech, float e_alpha, float e_beta, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    float omega_elec = omega_mech * params->pole_pairs;
    float e_mag = fSqrt(e_alpha * e_alpha + e_beta * e_beta);

    if (FABSF(omega_elec) > PSI_MIN_OMEGA && FABSF(e_mag) > PSI_MIN_E_MAG)
    {
        ctx->psi_ctx.valid_cnt++;
        ctx->psi_ctx.sum_e_mag += e_mag;
        ctx->psi_ctx.sum_omega += FABSF(omega_elec);
        if (ctx->psi_ctx.valid_cnt >= PSI_STEADY_TICKS)
            ctx->psi_ctx.ready = true;
    }
    else
    {
        ctx->psi_ctx.valid_cnt = 0;
        ctx->psi_ctx.ready = false;
    }

    if (ctx->psi_ctx.ready)
    {
        float avg_e = ctx->psi_ctx.sum_e_mag / (ctx->psi_ctx.valid_cnt + 1e-6f);
        float avg_omega = ctx->psi_ctx.sum_omega / (ctx->psi_ctx.valid_cnt + 1e-6f);
        params->Psi_f = avg_e / (avg_omega + 1e-6f);
        params->Ke = params->Psi_f * params->pole_pairs;
        if (params->Psi_f < 0.001f || params->Psi_f > 1.0f)
        {
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            return true;
        }
        params->psi_valid = true;
        return true;
    }
    return false;
}

/* ================================= 转动惯量/摩擦系数整定 ================================= */
static bool _tune_JB(float omega_mech, float iq_fb, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    // 阶段 1: 加速
    if (!ctx->jb_ctx.accel_phase)
    {
        // TODO: 施加阶跃转矩
        // fFOC_SetTargetIdIq(0, 2.0f);
        if (FABSF(omega_mech) > JB_MIN_OMEGA)
        {
            ctx->jb_ctx.accel_phase = true;
            ctx->jb_ctx.omega_start = omega_mech;
            ctx->jb_ctx.sample_cnt = 0;
            ctx->jb_ctx.sum_torque = 0;
            ctx->jb_ctx.sum_accel = 0;
        }
    }
    // 阶段 2: 采样
    else
    {
        static float omega_last = 0;
        float accel = (omega_mech - omega_last) / params->dt;
        omega_last = omega_mech;
        if (FABSF(accel) > JB_MIN_ACCEL)
        {
            ctx->jb_ctx.sum_torque += iq_fb * params->Ke;
            ctx->jb_ctx.sum_accel += accel;
            ctx->jb_ctx.sample_cnt++;
        }
        if (ctx->jb_ctx.sample_cnt >= 200)
            ctx->jb_ctx.ready = true; // 采样 10ms
    }

    if (ctx->jb_ctx.ready)
    {
        float avg_torque = ctx->jb_ctx.sum_torque / (ctx->jb_ctx.sample_cnt + 1e-6f);
        float avg_accel = ctx->jb_ctx.sum_accel / (ctx->jb_ctx.sample_cnt + 1e-6f);
        if (FABSF(avg_accel) > 1.0f)
            params->J = avg_torque / (avg_accel + 1e-6f);
        else
            params->J = 0.0001f;

        // 计算摩擦系数 (匀速阶段)
        // TODO: 切换到匀速控制
        // fFOC_SetTargetIdIq(0, 0.5f);
        // 简单估算
        params->B = 0.001f;
        if (params->J < 1e-6f || params->J > 0.01f)
        {
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            return true;
        }
        params->mech_valid = true;
        return true;
    }
    return false;
}

/* ================================= 辅助函数 ================================= */
static inline u32 _tune_elapsed_ticks(u32 start_tick)
{
    u32 now = g_tune_ctx.tick_count;
    return (now >= start_tick) ? (now - start_tick) : (0xFFFFFFFFU - start_tick + now);
}

/* ================================= 整定主循环 ================================= */
eTuneState fMotorParamTune_Update(tFOC_val foc_val)
{
    tTuneContext *ctx = &g_tune_ctx;
    tMotorParams *params = &g_motor_params;
    ctx->tick_count++;

    if (ctx->timeout_tick > 0 && ctx->tick_count > ctx->timeout_tick)
    {
        ctx->fault = TUNE_FAULT_TIMEOUT;
        ctx->state = TUNE_STATE_FAULT;
    }

    switch (ctx->state)
    {
    case TUNE_STATE_IDLE:
        // TODO: 设置初始模式
        // fFOC_SetSensorMode(ENCODER_CONTROL); fFOC_SetRunMode(IDLE_LOOP);
        ctx->rs_ctx.step = 0;
        ctx->rs_ctx.target_i[0] = RS_I_MIN;
        ctx->rs_ctx.target_i[1] = RS_I_MIN * 2.5f;
        ctx->state = TUNE_STATE_RS;
        break;
    case TUNE_STATE_RS:
        if (_tune_Rs(foc_val.Ualpha, foc_val.Ialpha, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_LS;
        }
        break;
    case TUNE_STATE_LS:
        if (_tune_Ls(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_THETA_OFFSET;
        }
        break;
    case TUNE_STATE_THETA_OFFSET:
        if (_tune_ThetaOffset(foc_val.theta_elec, foc_val.omega_fb, foc_val.id_fb, foc_val.iq_fb, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_WIRE_SEQ;
        }
        break;
    case TUNE_STATE_WIRE_SEQ:
        if (_tune_WireSequence(NULL, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_POLE_PAIRS;
        }
        break;
    case TUNE_STATE_POLE_PAIRS:
        if (_tune_PolePairs(foc_val.omega_fb, foc_val.omega_elec, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_PSI_F;
        }
        break;
    case TUNE_STATE_PSI_F:
        if (_tune_PsiF(foc_val.omega_fb, foc_val.e_alpha, foc_val.e_beta, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_JB;
        }
        break;
    case TUNE_STATE_JB:
        if (_tune_JB(foc_val.omega_fb, foc_val.iq_fb, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            ctx->state = TUNE_STATE_COMPLETE;
        }
        break;
    case TUNE_STATE_COMPLETE:
        // TODO: 主线程检测到此状态后调用 fParamTuneWrite()
        break;
    case TUNE_STATE_FAULT:
        break;
    default:
        break;
    }
    return ctx->state;
}

/* ================================= 辅助接口 ================================= */

eTuneFault fMotorParamTune_GetFault(void) { return g_tune_ctx.fault; }
