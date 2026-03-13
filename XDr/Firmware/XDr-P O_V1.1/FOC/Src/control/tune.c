#include "tune.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"
#include "filter.h"
/* ================================= 全局变量定义 ================================= */
tMotorParams motor_params = {0};
tTuneContext g_tune_ctx = {0};

tFirstOrderLagFilter rs_i_filter = {0};
#define rS_I_FILTER_ALPHA 0.3f

/* ================================= 整定参数配置 ================================= */
// 通用时间转换 (假设 20kHz 中断，1 tick = 50us)
#define TICK_TO_MS(tick) ((tick) * 0.05f)
#define MS_TO_TICK(ms) ((u16)((ms) * 20.0f))

// 电阻整定 (滞环电流控制 + 滤波 + 差分)
#define RS_TIMEOUT_TICKS 10000  // 整定超时时间 (500ms)
#define RS_I_TARGET_1 2.0f      // 第一点目标电流 (A)
#define RS_I_TARGET_2 6.0f      // 第二点目标电流 (A)
#define RS_HYST_BAND 1.5f       // 滞环带宽 ±1.5A
#define RS_V_LIMIT 2.0f         // 电压输出限幅 (V)
#define RS_V_HOLD_MAX_TICKS 5   // 误差带内保持最大周期数 (防静差)
#define RS_V_STEP_MIN 0.01f     // 保持超时后微调步长 (V)
#define RS_STEADY_ERR_THR 0.1f  // 稳态电流误差阈值 (A)
#define RS_STEADY_TICKS 150     // 稳态持续周期数 (7.5ms@20kHz)
#define RS_TRACK_TIMEOUT 800    // 单点跟踪超时 (40ms)
#define RS_MIN_DELTA_I 2.0f     // 最小电流变化量 (A)
#define RS_RANGE_MIN 0.02f      // 电阻合理下限 (Ω)
#define RS_RANGE_MAX 0.5f       // 电阻合理上限 (Ω)
#define RS_DEADTIME_VCOMP 0.04f // 死区补偿电压 (V)

// 电感整定
#define LS_TIMEOUT_TICKS 10000 // 整定超时时间 (500ms)
#define LS_INJECT_FREQ_HZ 1000
#define LS_V_HOLD_MAX_TICKS 100
#define LS_V_START 0.2f
#define LS_V_MAX 0.8f
#define LS_I_LIMIT 4.0f
#define LS_MIN_DI_DT 300.0f // 最小信噪比要求
#define LS_MAX_DI_DT 60000.0f
#define LS_RANGE_MIN 20e-6f
#define LS_RANGE_MAX 300e-6f

// 角度偏移 (开环强励磁)
#define THETA_TIMEOUT_TICKS 10000 // 整定超时时间 (500ms)
#define THETA_VOLT_AMP 0.4f       // 强励磁电压幅值 (V)
#define THETA_DELTA_MAX 0.01f     // 静止判断阈值 (rad)
#define THETA_STEADY_WIN 1000     // 静止等待时间 (50ms)

// 线序
#define WS_TIMEOUT_TICKS 100000 // 整定超时时间 (5s)
#define WS_VF_FREQ 3.0f
#define WS_VF_AMP 1.5f
#define WS_DURATION_TICKS MS_TO_TICK(500)
#define WS_WAIT_STOP_TICKS MS_TO_TICK(300)
#define WS_ENC_DELTA_MIN 50

/* ================================= 辅助函数 ================================= */
static inline u32 _tune_elapsed_ticks(u32 start_tick)
{
    u32 now = g_tune_ctx.tick_count;
    return (now >= start_tick) ? (now - start_tick) : (0xFFFFFFFFU - start_tick + now);
}

/* ================================= 参数访问实现 ================================= */
void fMotorParamTune_ForceSave(void)
{
    // 过程中会直接写入motor用于后续控制
    //  这里是集中写入参数flash中转站
    g_Param.motor_rs = motor_params.Rs;
    g_Param.motor_ld = motor_params.Ld;
    g_Param.motor_lq = motor_params.Lq;
    g_Param.motor_psif = motor_params.Psi_f;
    g_Param.motor_ke = motor_params.Ke;
    g_Param.motor_j = motor_params.J;
    g_Param.motor_b = motor_params.B;
    g_Param.motor_polepairs = motor_params.pole_pairs;
    g_Param.motor_wire_sequence = motor_params.wire_sequence ? 0 : 1;
    g_Param.theta_offset = motor_params.theta_offset;
}

/* ================================= 初始化与重置 ================================= */
// todo:后续添加无感整定，跳过角度偏移和极对数整定，用电角度和电角速度控制，直接用HFI、SMO做精准的控制
void fMotorParamTune_Init()
{

    memset(&g_tune_ctx, 0, sizeof(tTuneContext));

    motor_params.KV = g_Param.motor_kv;
    motor_params.Rs = g_Param.motor_rs;
    motor_params.Ld = g_Param.motor_ld;
    motor_params.Lq = g_Param.motor_lq;
    motor_params.pole_pairs = g_Param.motor_polepairs;
    motor_params.dt = Tcon;
    motor_params.wire_sequence = true;
    fSetWireSequence(motor_params.wire_sequence);

    // 标记未整定
    motor_params.Rs_valid = false;
    motor_params.L_valid = false;
    motor_params.offset_valid = false;
    motor_params.wire_valid = false;
    motor_params.pole_valid = false;
    motor_params.psi_valid = false;
    motor_params.mech_valid = false;

    // 滤波器初始化
    fFirstOrderLagInit(&rs_i_filter, rS_I_FILTER_ALPHA, 0);
}

void fMotorParamTune_Reset()
{
    g_tune_ctx.fault = TUNE_FAULT_NONE;
    g_tune_ctx.state = TUNE_STATE_INIT;
}
/* ================================= 电阻整定 (开环 + 滤波 + 差分) ================================= */

static bool _tune_Rs(float i_alpha_fb, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;

    float i_alpha = fFirstOrderLagFilter(&rs_i_filter, i_alpha_fb);
    //===滞环电流控制===
    float err = ctx->rs_ctx.i_target - i_alpha;
    if (FABSF(err) > RS_HYST_BAND)
    {
        // 误差带外 → 输出电压粗调
        ctx->rs_ctx.v_cmd += (err > 0) ? 10 * RS_V_STEP_MIN : -10 * RS_V_STEP_MIN;
        ctx->rs_ctx.v_cmd = CLAMP(ctx->rs_ctx.v_cmd, -RS_V_LIMIT, RS_V_LIMIT);
        ctx->rs_ctx.hold_cnt = 0;
    }
    else
    {
        // 误差带内 → 保持输出，微调电压，但限制保持时间防静差
        if (++ctx->rs_ctx.hold_cnt >= RS_V_HOLD_MAX_TICKS)
        {
            ctx->rs_ctx.v_cmd += (err > 0) ? RS_V_STEP_MIN : -RS_V_STEP_MIN;
            ctx->rs_ctx.v_cmd = CLAMP(ctx->rs_ctx.v_cmd, -RS_V_LIMIT, RS_V_LIMIT);
            ctx->rs_ctx.hold_cnt = 0;
        }
    }
    //===死区前馈===
    float v_dead_comp = (i_alpha > 0) ? RS_DEADTIME_VCOMP : -RS_DEADTIME_VCOMP;
    float v_out = CLAMP(ctx->rs_ctx.v_cmd + v_dead_comp, -RS_V_LIMIT, RS_V_LIMIT);

    // 稳态判断
    float i_err = FABSF(i_alpha - ctx->rs_ctx.i_target);
    if (i_err < RS_STEADY_ERR_THR)
    {
        if (++ctx->tick_count >= RS_STEADY_TICKS)
        {
            // === 4. 数据采集 ===
            if (ctx->rs_ctx.step == 0)
            {
                // 记录第一点
                ctx->rs_ctx.i_meas[0] = i_alpha;
                ctx->rs_ctx.v_meas[0] = v_out; // 记录实际输出电压

                // 切换到第二点
                ctx->rs_ctx.step = 1;
                ctx->tick_count = 0;
                ctx->rs_ctx.step_ticks = 0;

                // 重置滞环控制器
                ctx->rs_ctx.i_target = RS_I_TARGET_2;
                ctx->rs_ctx.v_cmd = 0;
                ctx->rs_ctx.hold_cnt = 0;

                return false; // 继续第二点
            }
            else
            {
                // 记录第二点
                ctx->rs_ctx.i_meas[1] = i_alpha;
                ctx->rs_ctx.v_meas[1] = v_out;

                // === 5. 差分计算电阻 ===
                float delta_i = ctx->rs_ctx.i_meas[1] - ctx->rs_ctx.i_meas[0];
                float delta_v = ctx->rs_ctx.v_meas[1] - ctx->rs_ctx.v_meas[0];

                // 信噪比检查
                if (FABSF(delta_i) < RS_MIN_DELTA_I)
                {
                    ctx->fault = TUNE_FAULT_SIGNAL_WEAK;
                    return true;
                }

                // 计算电阻
                params->Rs = delta_v / (delta_i + 1e-6f);

                // 合理性校验
                if (params->Rs < RS_RANGE_MIN || params->Rs > RS_RANGE_MAX)
                {
                    ctx->fault = TUNE_FAULT_PARAM_INVALID;
                    return true;
                }

                params->Rs_valid = true;
                fFOC_SetUalphaBeta(0, 0);
                return true; // 辨识完成
            }
        }
    }
    else
    {
        // 电流波动，重置稳态计数
        ctx->tick_count = 0;
    }

    // === 6. 输出电压 ===
    fFOC_SetUalphaBeta(v_out, 0);

    return false; // 辨识进行中
}
/* ================================= 电感整定（alpha beta轴方波注入） ================================= */
static bool
_tune_Ls(float v_alpha, float v_beta, float i_alpha, float i_beta, tMotorParams *params)
{

    tTuneContext *ctx = &g_tune_ctx;

    // 自适应注入电压 调整到反馈电流合适
    if (!ctx->ls_ctx.ready)
    {
        float i_meas = (FABSF(i_alpha) + FABSF(i_beta)) * 0.5f;

        // 迟滞区间: 20%~80% of LS_I_LIMIT
        if (i_meas < LS_I_LIMIT * 0.2f && ctx->ls_ctx.v_inj < LS_V_MAX - 0.1f)
        {
            ctx->ls_ctx.v_inj += 0.1f;
            ctx->tick_count = 0;
        }
        else if (i_meas > LS_I_LIMIT * 0.8f && ctx->ls_ctx.v_inj > LS_V_START + 0.1f)
        {
            ctx->ls_ctx.v_inj -= 0.1f;
            ctx->tick_count = 0;
        }
        else
        {
            if (ctx->tick_count > LS_V_HOLD_MAX_TICKS)
            {
                ctx->ls_ctx.ready = true;
                ctx->tick_count = 0;
                fFOC_SetUalphaBeta(0, 0);
                return false;
            }
        }
        fFOC_SetUalphaBeta(ctx->ls_ctx.v_inj, ctx->ls_ctx.v_inj);
        return false; // 未完成
    }

    // 反转极性
    if (++ctx->ls_ctx.inject_cnt >= ctx->ls_ctx.inject_period)
    {
        ctx->ls_ctx.inject_cnt = 0;
        ctx->temp_flag[0] = !ctx->temp_flag[0]; // 注入极性翻转
    }
    // 施加电压
    float v_cmd = ctx->temp_flag[0] ? ctx->ls_ctx.v_inj : -ctx->ls_ctx.v_inj;
    fFOC_SetUalphaBeta(v_cmd, v_cmd);

    if (ctx->ls_ctx.inject_cnt == 0)
    {
        float di_alpha = i_alpha - ctx->temp_val[0];
        float di_beta = i_beta - ctx->temp_val[1];
        float dt_actual = ctx->ls_ctx.inject_period * params->dt;
        float di_dt_alpha = di_alpha / dt_actual;
        float di_dt_beta = di_beta / dt_actual;
        // 记录有效样本
        u8 half = ctx->temp_flag[0] ? 1 : 0;

        if (FABSF(v_cmd) > ctx->ls_ctx.v_inj * 0.9f)
        {
            // 记录alpha轴
            if (FABSF(di_dt_alpha) > LS_MIN_DI_DT &&
                FABSF(di_dt_alpha) < LS_MAX_DI_DT)
            {
                ctx->ls_ctx.di_dt_sum[0][half] += di_dt_alpha;
                ctx->ls_ctx.cnt[0][half]++;
                ctx->ls_ctx.i_sum[0][half] += i_alpha;
            }
            if (FABSF(di_dt_beta) > LS_MIN_DI_DT &&
                FABSF(di_dt_beta) < LS_MAX_DI_DT)
            {
                ctx->ls_ctx.di_dt_sum[1][half] += di_dt_beta;
                ctx->ls_ctx.cnt[1][half]++;
                ctx->ls_ctx.i_sum[1][half] += i_beta;
            }
        }
        ctx->temp_val[0] = i_alpha;
        ctx->temp_val[1] = i_beta;
    }

    // 完成判断
    if (ctx->tick_count >= 4000)
    {
        // 计算电感
        float L_alpha = 0, L_beta = 0;
        bool alpha_ok = (ctx->ls_ctx.cnt[0][0] >= 20 && ctx->ls_ctx.cnt[0][1] >= 20);
        bool beta_ok = (ctx->ls_ctx.cnt[1][0] >= 20 && ctx->ls_ctx.cnt[1][1] >= 20);

        if (alpha_ok)
        {
            float avg_di_dt = (FABSF(ctx->ls_ctx.di_dt_sum[0][0] / ctx->ls_ctx.cnt[0][0]) +
                               FABSF(ctx->ls_ctx.di_dt_sum[0][1] / ctx->ls_ctx.cnt[0][1])) *
                              0.5f;
            // float i_avg = (ctx->ls_ctx.i_sum[0][0] / ctx->ls_ctx.cnt[0][0] +
            //                ctx->ls_ctx.i_sum[0][1] / ctx->ls_ctx.cnt[0][1]) *
            //               0.5f;
            float i_avg_0 = FABSF(ctx->ls_ctx.i_sum[0][0] / ctx->ls_ctx.cnt[0][0]);
            float i_avg_1 = FABSF(ctx->ls_ctx.i_sum[0][1] / ctx->ls_ctx.cnt[0][1]);
            float i_avg = (i_avg_0 + i_avg_1) * 0.5f;
            float v_comp = ctx->ls_ctx.v_inj - params->Rs * i_avg; // 电阻压降补偿
            L_alpha = v_comp / (avg_di_dt + 1e-6f);
        }

        if (beta_ok)
        {
            float avg_di_dt = (FABSF(ctx->ls_ctx.di_dt_sum[1][0] / ctx->ls_ctx.cnt[1][0]) +
                               FABSF(ctx->ls_ctx.di_dt_sum[1][1] / ctx->ls_ctx.cnt[1][1])) *
                              0.5f;
            float i_avg_0 = FABSF(ctx->ls_ctx.i_sum[1][0] / ctx->ls_ctx.cnt[1][0]);
            float i_avg_1 = FABSF(ctx->ls_ctx.i_sum[1][1] / ctx->ls_ctx.cnt[1][1]);
            float i_avg = (i_avg_0 + i_avg_1) * 0.5f;
            // float i_avg = (ctx->ls_ctx.i_sum[1][0] / ctx->ls_ctx.cnt[1][0] +
            //                ctx->ls_ctx.i_sum[1][1] / ctx->ls_ctx.cnt[1][1]) *
            //               0.5f;
            float v_comp = ctx->ls_ctx.v_inj - params->Rs * i_avg;
            L_beta = v_comp / (avg_di_dt + 1e-6f);
        }

        // 融合结果
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

        // 合理性校验
        if (params->Ld < LS_RANGE_MIN || params->Ld > LS_RANGE_MAX)
        {
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            return true;
        }

        params->L_valid = true;
        return true;
    }

    return false; // 进行中
}

/* ================================= 角度偏移整定 (开环强励磁) ================================= */
static bool _tune_ThetaOffset(float theta_mech, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;

    // 稳态检测：机械角速度接近 0
    float theta_delta = fNormalizeAngle_pi_pi(theta_mech - ctx->temp_val[0]);
    if (FABSF(theta_delta) < THETA_DELTA_MAX)
    {
        ctx->theta_ctx.valid_cnt++;
        if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN)
        {
            ctx->theta_ctx.theta_sum += theta_mech; // 滑动平均
            if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 2 - 1)
                ctx->theta_ctx.steady_flag = true;
        }
    }
    else
    {
        ctx->theta_ctx.valid_cnt = 0;
        ctx->theta_ctx.theta_sum = 0;
    }
    ctx->temp_val[0] = theta_mech;

    // 多周期验证
    if (ctx->theta_ctx.steady_flag)
    {
        float offset = ctx->theta_ctx.theta_sum / THETA_STEADY_WIN;
        offset = fNormalizeAngle_pi_pi(offset);

        params->theta_offset = offset;
        params->offset_valid = true;
        return true;
    }
    return false; // 进行中
}

/* ================================= 线序整定 (正反开环测试) ================================= */
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
        fSetWireSequence(false); // 反转线序
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
            params->wire_sequence = true; // 正序
            ctx->wire_ctx.step = 5;
        }
        else if (ctx->wire_ctx.enc_delta_fwd < -WS_ENC_DELTA_MIN && ctx->wire_ctx.enc_delta_rev > WS_ENC_DELTA_MIN)
        {
            params->wire_sequence = false; // 反序
            ctx->wire_ctx.step = 5;
        }
        else
        {
            ctx->fault = TUNE_FAULT_MECH_LOCKED;
            return true;
        }
        break;
    case 5: // 完成
        params->wire_valid = true;
        return true;
    }
    return false;
}

/* ================================= 极对数整定 (HFI 框架) ================================= */
static bool _tune_PolePairs(tFOC_val foc_val, tMotorParams *params)
{
    // TODO: HFI 低速注入框架
    // 1. 低速运行HFI
    // 2. 从HFI获取电角度信息，从编码器获取机械角度信息
    // 3. 计算极对数: omega_elec/omega_mech

    params->pole_valid = true;
    return true;
}

/* ================================= 磁链整定 (SMO 框架) ================================= */
static bool _tune_PsiF(tFOC_val foc_val, tMotorParams *params)
{
    // TODO: SMO 高速整定框架
    // 1. 高速运行 (例如 1000rpm+)，SMO 估算反电动势
    // 2. Ke = |E| / omega_elec
    // 3. Psi_f = Ke / pole_pairs

    // 临时占位：使用 KV 反推（后续替换为 SMO 实测）
    params->Ke = 60.0f / (2.0f * MATH_PI * motor_params.KV * params->pole_pairs);
    params->Psi_f = params->Ke / params->pole_pairs;
    params->psi_valid = true;
    return true;
}

/* ================================= 转动惯量/摩擦系数整定 (框架) ================================= */
static bool _tune_JB(tFOC_val foc_val, tMotorParams *params)
{
    // TODO: 阶跃响应法框架
    // 1. 施加阶跃转矩 (例如 iq=2A)
    // 2. 记录加速度曲线: alpha = d(omega)/dt
    // 3. J = T / alpha (忽略摩擦), B = (T - J*alpha) / omega (匀速段)

    // 临时占位：使用默认值跳过
    params->J = 0.0001f;
    params->B = 0.001f;
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
    case TUNE_STATE_INIT:
        fMotorParamTune_Init();
        ctx->state = TUNE_STATE_IDLE;
        break;
    case TUNE_STATE_IDLE:
        fFOC_SetSensorMode(ENCODER_CONTROL);
        fFOC_SetRunMode(OPEN_LOOP);
        // 准备工作：初始化电阻整定上下文
        ctx->rs_ctx.i_target = RS_I_TARGET_1;
        ctx->rs_ctx.v_cmd = 0;
        ctx->rs_ctx.step = 0;
        ctx->temp_val[0] = 0;
        ctx->temp_val[1] = 0;
        ctx->tick_count = 0;

        ctx->timeout_tick = RS_TIMEOUT_TICKS;
        ctx->state = TUNE_STATE_RS;
        break;

    case TUNE_STATE_RS:
        if (_tune_Rs(foc_val.Ialpha, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            // 电阻完成：初始化电感整定上下文
            ctx->ls_ctx.inject_period = fpwm / LS_INJECT_FREQ_HZ / 2;
            ctx->ls_ctx.v_inj = LS_V_START;
            ctx->ls_ctx.inject_cnt = 0;
            for (int i = 0; i < 2; i++)
                for (int j = 0; j < 2; j++)
                {
                    ctx->ls_ctx.di_dt_sum[i][j] = 0;
                    ctx->ls_ctx.cnt[i][j] = 0;
                    ctx->ls_ctx.i_sum[i][j] = 0;
                }
            ctx->tick_count = 0;

            ctx->timeout_tick = LS_TIMEOUT_TICKS;
            ctx->state = TUNE_STATE_LS;
        }
        break;

    case TUNE_STATE_LS:
        if (_tune_Ls(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta, params))
        {
            // 电阻上下文复位
            ctx->rs_ctx.step_ticks = 0;

            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            // todo:如果开启自适应PID，这里对电流环PI进行调节

            // 电感完成：初始化角度偏移上下文
            ctx->theta_ctx.theta_sum = 0;
            ctx->theta_ctx.valid_cnt = 0;
            ctx->theta_ctx.steady_flag = false;
            ctx->tick_count = 0;
            ctx->temp_val[0] = foc_val.theta_mech; // 提前储存一次机械角度

            fFOC_SetUalphaBeta(THETA_VOLT_AMP, 0);

            ctx->timeout_tick = THETA_TIMEOUT_TICKS;
            ctx->state = TUNE_STATE_THETA_OFFSET;
        }
        break;

    case TUNE_STATE_THETA_OFFSET:
        if (_tune_ThetaOffset(foc_val.theta_mech, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            fSetThetaOffset(motor_params.theta_offset); // 应用于目前计算

            // 角度完成：初始化线序上下文
            ctx->wire_ctx.step = 0;
            ctx->wire_ctx.enc_delta_fwd = 0;
            ctx->wire_ctx.enc_delta_rev = 0;
            ctx->tick_count = 0;

            ctx->timeout_tick = WS_TIMEOUT_TICKS;
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
            fSetWireSequence(motor_params.wire_sequence);
            // 线序完成：初始化极对数上下文
            // todo:这里切换为混合模式，使用编码器和HFI的值来确定极对数

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

            //  极对数完成：初始化磁链上下文
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

            //  磁链完成：初始化机械参数上下文
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

            // todo:这里可以对速度环PI和位置环PID 参数进行调节
            //  结束：保存参数并进入完成状态

            fMotorParamTune_ForceSave();
            ctx->state = TUNE_STATE_COMPLETE;
        }
        break;

    default:
        break;
    }
    return ctx->state;
}

/* ================================= 辅助接口 ================================= */
eTuneFault fMotorParamTune_GetFault(void) { return g_tune_ctx.fault; }