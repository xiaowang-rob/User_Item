#include "tune.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"

/* ================================= 全局变量定义 ================================= */
tMotorParams motor_params = {0};
tTuneContext g_tune_ctx = {0};

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

// 角度偏移 (开环强励磁)
#define THETA_VOLT_AMP 2.0f   // 强励磁电压幅值 (V)
#define THETA_OMEGA_MAX 0.05f // 静止判断阈值 (rad/s)
#define THETA_STEADY_WIN 500  // 静止等待时间 (25ms)

// 线序
#define WS_VF_FREQ 3.0f
#define WS_VF_AMP 1.5f
#define WS_DURATION_TICKS MS_TO_TICK(500)
#define WS_WAIT_STOP_TICKS MS_TO_TICK(300)
#define WS_ENC_DELTA_MIN 50

// 极对数 (HFI 框架)
// TODO: HFI 参数定义

// 磁链 (SMO 框架)
// TODO: SMO 参数定义

// 机械参数 (框架)
// TODO: JB 参数定义

/* ================================= 辅助函数 ================================= */
static inline u32 _tune_elapsed_ticks(u32 start_tick)
{
    u32 now = g_tune_ctx.tick_count;
    return (now >= start_tick) ? (now - start_tick) : (0xFFFFFFFFU - start_tick + now);
}


/* ================================= 参数访问实现 ================================= */

void fMotorParamTune_ForceSave(void)
{
    // TODO: 这里全置位是为了调试兜底，正常流程应该是每步单独置位
    g_Param.motor_rs = motor_params.Rs;
    g_Param.motor_ld = motor_params.Ld;
    g_Param.motor_lq = motor_params.Lq;
    g_Param.motor_psif = motor_params.Psi_f;
    g_Param.motor_ke = motor_params.Ke;
    g_Param.motor_j = motor_params.J;
    g_Param.motor_b = motor_params.B;
    g_Param.motor_polepairs = motor_params.pole_pairs;
    // 线序、偏移角度在整定过程中就写入了
}

/* ================================= 初始化与重置 ================================= */
void fMotorParamTune_Init()
{
    fMotorParamTune_Reset();

    motor_params.KV = g_Param.motor_kv;
    motor_params.Rs = g_Param.motor_rs;
    motor_params.Ld = g_Param.motor_ld;
    motor_params.Lq = g_Param.motor_lq;
    motor_params.pole_pairs = g_Param.motor_polepairs;
    motor_params.dt = Tcon;
    motor_params.wire_sequence = 1;

    // 标记未整定
    motor_params.Rs_valid = false;
    motor_params.L_valid = false;
    motor_params.offset_valid = false;
    motor_params.wire_valid = false;
    motor_params.pole_valid = false;
    motor_params.psi_valid = false;
    motor_params.mech_valid = false;
}

void fMotorParamTune_Reset(void)
{
    g_tune_ctx.state = TUNE_STATE_IDLE;
    g_tune_ctx.fault = TUNE_FAULT_NONE;
    g_tune_ctx.tick_count = 0;
    g_tune_ctx.timeout_tick = 0;
    // 其他变量在状态切换时初始化，这里不重复清零
}

/* ================================= 电阻整定 ================================= */
static bool _tune_Rs(float v_alpha, float i_alpha, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    float target_i = ctx->rs_ctx.target_i[ctx->rs_ctx.step];
    float i_err = target_i - i_alpha;
    float v_cmd = i_err * 0.3f;
    v_cmd = CLAMP(v_cmd, -RS_V_LIMIT, RS_V_LIMIT);

    fFOC_SetUalphaBeta(v_cmd, 0);
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

    if (!ctx->ls_ctx.axis)
        fFOC_SetUalphaBeta(v_cmd, 0);
    else
        fFOC_SetUalphaBeta(0, v_cmd);

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

/* ================================= 角度偏移整定 (开环强励磁版) ================================= */
static bool _tune_ThetaOffset(float theta_mech, float omega_mech, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;

    fFOC_SetUalphaBeta(THETA_VOLT_AMP, 0);

    // 稳态检测：机械角速度接近 0
    if (FABSF(omega_mech) < THETA_OMEGA_MAX)
    {
        ctx->theta_ctx.valid_cnt++;
        if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN)
        {
            ctx->theta_ctx.theta_sum += theta_mech * 0.01f; // 滑动平均
            if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 2)
                ctx->theta_ctx.steady_flag = true;
        }
    }
    else
    {
        // 非稳态：衰减计数
        if (ctx->theta_ctx.valid_cnt > 10)
            ctx->theta_ctx.valid_cnt -= 10;
        else
            ctx->theta_ctx.valid_cnt = 0;
        ctx->theta_ctx.theta_sum = 0;
    }

    // 多周期验证
    if (ctx->theta_ctx.steady_flag && ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 3)
    {
        float offset = ctx->theta_ctx.theta_sum / (THETA_STEADY_WIN * 3 * 0.01f);
        offset = fNormalizeAngle_pi_pi(offset);

        // TODO:补偿到编码器输出端
        params->theta_offset = offset;
        params->offset_valid = true;
        return true;
    }
    return false;
}

/* ================================= 线序整定 (不关 PWM 版) ================================= */
static bool _tune_WireSequence(tFOC_val foc_val, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;

    switch (ctx->wire_ctx.step)
    {
    case 0:                                                         // 初始化：记录起始机械角度
        ctx->wire_ctx.enc_start = (s32)(foc_val.theta_mech * 1000); // 放大便于整数比较
        ctx->wire_ctx.step = 1;
        ctx->wire_ctx.step_start_tick = g_tune_ctx.tick_count;
        break;

    case 1: // 正向开环拖动
        if (_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick) < WS_DURATION_TICKS)
        {
            float elapsed_ms = TICK_TO_MS(_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick));
            float angle = 2.0f * MATH_PI * WS_VF_FREQ * elapsed_ms * 0.001f;
            float Valpha = WS_VF_AMP * cosf(angle);
            float Vbeta = WS_VF_AMP * sinf(angle);
            // TODO: IDLE_LOOP 模式下开环输出电压
            fFOC_SetUalphaBeta(Valpha, Vbeta);
        }
        else
        {
            // 拖动结束，记录正向角度变化
            s32 enc_now = (s32)(foc_val.theta_mech * 1000);
            ctx->wire_ctx.enc_delta_fwd = enc_now - ctx->wire_ctx.enc_start;
            ctx->wire_ctx.step = 2;
            ctx->wire_ctx.step_start_tick = g_tune_ctx.tick_count;
            ctx->wire_ctx.wait_ticks = WS_WAIT_STOP_TICKS;
        }
        break;

    case 2: // 等待静止
        if (_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick) < ctx->wire_ctx.wait_ticks)
            break;
        // 记录反向拖动前的角度
        ctx->wire_ctx.enc_start = (s32)(foc_val.theta_mech * 1000);
        ctx->wire_ctx.step = 3;
        ctx->wire_ctx.step_start_tick = g_tune_ctx.tick_count;
        break;

    case 3: // 反向开环拖动
        if (_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick) < WS_DURATION_TICKS)
        {
            float elapsed_ms = TICK_TO_MS(_tune_elapsed_ticks(ctx->wire_ctx.step_start_tick));
            float angle = -2.0f * MATH_PI * WS_VF_FREQ * elapsed_ms * 0.001f; // 反向
            float Valpha = WS_VF_AMP * cosf(angle);
            float Vbeta = WS_VF_AMP * sinf(angle);
            fFOC_SetUalphaBeta(Valpha, Vbeta);
        }
        else
        {
            // 记录反向角度变化
            s32 enc_now = (s32)(foc_val.theta_mech * 1000);
            ctx->wire_ctx.enc_delta_rev = enc_now - ctx->wire_ctx.enc_start;
            ctx->wire_ctx.step = 4;
        }
        break;

    case 4: // 判断线序
        if (ctx->wire_ctx.enc_delta_fwd > WS_ENC_DELTA_MIN && ctx->wire_ctx.enc_delta_rev < -WS_ENC_DELTA_MIN)
        {
            params->wire_sequence = 1; // 正序
            ctx->wire_ctx.step = 5;
        }
        else if (ctx->wire_ctx.enc_delta_fwd < -WS_ENC_DELTA_MIN && ctx->wire_ctx.enc_delta_rev > WS_ENC_DELTA_MIN)
        {
            params->wire_sequence = -1; // 反序
            ctx->wire_ctx.step = 5;
        }
        else
        {
            ctx->fault = TUNE_FAULT_MECH_LOCKED;
            return true;
        }
        break;

    case 5: // 应用线序
        fSetWireSequence(params->wire_sequence);
        params->wire_valid = true;
        return true;
    }
    return false;
}

/* ================================= 极对数整定 (HFI 框架) ================================= */
static bool _tune_PolePairs(tFOC_val foc_val, tMotorParams *params)
{
    // TODO: HFI 低速注入框架
    // 1. 注入高频电压信号
    // 2. 检测电流响应相位
    // 3. 计算极对数
    // 示例占位：
    // if (hfi_detect_complete) {
    //     params->pole_pairs = hfi_result;
    //     params->pole_valid = true;
    //     return true;
    // }

    params->pole_valid = true;
    return true;
}

/* ================================= 磁链整定 (SMO 框架) ================================= */
static bool _tune_PsiF(tFOC_val foc_val, tMotorParams *params)
{
    // TODO: SMO 高速整定框架
    // 1. 高速运行，SMO 估算反电动势
    // 2. Ke = E / omega_elec
    // 3. Psi_f = Ke / pole_pairs
    // 示例占位：
    // if (smo_converged) {
    //     params->Ke = smo_Ke;
    //     params->Psi_f = params->Ke / params->pole_pairs;
    //     params->psi_valid = true;
    //     return true;
    // }

    params->psi_valid = true;
    return true;
}

/* ================================= 转动惯量/摩擦系数整定 (框架) ================================= */
static bool _tune_JB(tFOC_val foc_val, tMotorParams *params)
{
    // TODO: 阶跃响应法框架
    // 1. 施加阶跃转矩
    // 2. 记录加速度曲线
    // 3. J = T / alpha, B = (T - J*alpha) / omega
    // 示例占位：
    // if (jb_calc_done) {
    //     params->J = jb_J;
    //     params->B = jb_B;
    //     params->mech_valid = true;
    //     return true;
    // }

    params->mech_valid = true;
    return true;
}


/* ================================= 整定主循环 (状态切换时初始化) ================================= */
eTuneState fMotorParamTune_Update(tFOC_val foc_val)
{
    tTuneContext *ctx = &g_tune_ctx;
    tMotorParams *params = &motor_params;
    ctx->tick_count++;

    if (ctx->timeout_tick > 0 && ctx->tick_count > ctx->timeout_tick)
    {
        ctx->fault = TUNE_FAULT_TIMEOUT;
        ctx->state = TUNE_STATE_FAULT;
    }

    switch (ctx->state)
    {
    case TUNE_STATE_IDLE:
        // TODO: 设置初始模式 (IDLE_LOOP + ENCODER_CONTROL)
        // fFOC_SetSensorMode(ENCODER_CONTROL);
        // fFOC_SetRunMode(IDLE_LOOP);

        // ✅ 状态切换时初始化下一个状态的变量
        ctx->rs_ctx.step = 0;
        ctx->rs_ctx.target_i[0] = RS_I_MIN;
        ctx->rs_ctx.target_i[1] = RS_I_MIN * 2.5f;
        ctx->tick_count = 0; // 为下一个状态重置计时

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

            // ✅ 电阻完成：初始化电感整定上下文
            ctx->ls_ctx.v_inj = LS_V_START;
            ctx->ls_ctx.axis = false;
            ctx->ls_ctx.inject_cnt = 0;
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 2; j++)
                {
                    ctx->ls_ctx.di_dt_sum[i][j] = 0;
                    ctx->ls_ctx.cnt[i][j] = 0;
                    ctx->ls_ctx.i_peak[i][j] = 0;
                }
            ctx->tick_count = 0;

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

            // ✅ 电感完成：初始化角度偏移上下文
            ctx->theta_ctx.theta_sum = 0;
            ctx->theta_ctx.valid_cnt = 0;
            ctx->theta_ctx.steady_flag = false;
            ctx->tick_count = 0;

            ctx->state = TUNE_STATE_THETA_OFFSET;
        }
        break;

    case TUNE_STATE_THETA_OFFSET:
        if (_tune_ThetaOffset(foc_val.theta_mech, foc_val.omega_fb, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            // ✅ 角度完成：初始化线序上下文
            ctx->wire_ctx.step = 0;
            ctx->wire_ctx.enc_delta_fwd = 0;
            ctx->wire_ctx.enc_delta_rev = 0;
            ctx->tick_count = 0;

            ctx->state = TUNE_STATE_WIRE_SEQ;
        }
        break;

    case TUNE_STATE_WIRE_SEQ:
        if (_tune_WireSequence(foc_val, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            // ✅ 线序完成：初始化极对数上下文
            ctx->pole_ctx.omega_ref = 200.0f;
            ctx->pole_ctx.sum_ratio = 0;
            ctx->pole_ctx.valid_cnt = 0;
            ctx->pole_ctx.steady_flag = false;
            ctx->tick_count = 0;

            ctx->state = TUNE_STATE_POLE_PAIRS;
        }
        break;

    case TUNE_STATE_POLE_PAIRS:
        if (_tune_PolePairs(foc_val, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            // ✅ 极对数完成：初始化磁链上下文
            ctx->psi_ctx.sum_e_mag = 0;
            ctx->psi_ctx.sum_omega = 0;
            ctx->psi_ctx.valid_cnt = 0;
            ctx->psi_ctx.ready = false;
            ctx->tick_count = 0;

            ctx->state = TUNE_STATE_PSI_F;
        }
        break;

    case TUNE_STATE_PSI_F:
        if (_tune_PsiF(foc_val, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            // ✅ 磁链完成：初始化机械参数上下文
            ctx->jb_ctx.accel_phase = false;
            ctx->jb_ctx.sample_cnt = 0;
            ctx->jb_ctx.sum_torque = 0;
            ctx->jb_ctx.sum_accel = 0;
            ctx->jb_ctx.ready = false;
            ctx->tick_count = 0;

            ctx->state = TUNE_STATE_JB;
        }
        break;

    case TUNE_STATE_JB:
        if (_tune_JB(foc_val, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            fMotorParamTune_ForceSave();
            ctx->state = TUNE_STATE_COMPLETE;
        }
        break;

    case TUNE_STATE_COMPLETE:

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