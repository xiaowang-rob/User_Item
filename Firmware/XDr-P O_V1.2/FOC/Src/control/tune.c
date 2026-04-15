#include "tune.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"
#include "filter.h"
/* ================================= 全局变量定义 ================================= */
tMotorParams motor_params = {0};
tTuneContext g_tune_ctx = {0};

/* ================================= 整定参数配置 ================================= */
// 通用时间转换 (假设 20kHz 中断，1 tick = 50us)
#define TICK_TO_MS(tick) ((tick) * 0.05f)
#define MS_TO_TICK(ms) ((u16)((ms) * 20.0f))

#define TUNE_WAIT_TICKS MS_TO_TICK(1000) // 先静止等待时间 (1s)

// 电阻整定 (滞环电流控制 + 滤波 + 差分)
#define RS_FREQ_F 10                     // 电阻整定分频系数
#define RS_TIMEOUT_TICKS MS_TO_TICK(500) // 整定超时时间 (500ms)
#define RS_I_TARGET_1 3.0f               // 第一点目标电流 (A)
#define RS_I_TARGET_2 7.0f               // 第二点目标电流 (A)
#define RS_HYST_BAND 2.0f                // 滞环带宽 ±1.5A
#define RS_V_LIMIT 2.0f                  // 电压输出限幅 (V)
#define RS_V_HOLD_MAX_TICKS 5            // 误差带内保持最大周期数 (防静差)
#define RS_V_STEP_MIN 0.01f              // 保持超时后微调步长 (V)
#define RS_STEADY_ERR_THR 0.4f           // 稳态电流误差阈值 (A)
#define RS_STEADY_TICKS MS_TO_TICK(7)    // 稳态持续周期数 (7ms@20kHz)
#define RS_TRACK_TIMEOUT MS_TO_TICK(40)  // 单点跟踪超时 (40ms)
#define RS_MIN_DELTA_I 2.0f              // 最小电流变化量 (A)
#define RS_RANGE_MIN 0.02f               // 电阻合理下限 (Ω)
#define RS_RANGE_MAX 0.5f                // 电阻合理上限 (Ω)
#define RS_DEADTIME_VCOMP 0.04f          // 死区补偿电压 (V)

// 电感整定
#define LS_TIMEOUT_TICKS MS_TO_TICK(500) // 整定超时时间 (500ms)
#define LS_INJECT_FREQ_HZ 2500
#define LS_V_HOLD_MAX_TICKS 100 //
#define LS_V_START 0.2f
#define LS_V_MAX 0.8f
#define LS_I_LIMIT 3.0f
#define LS_I_STEP_MIN 0.04f
#define LS_MIN_DI_DT 100.0f // 最小信噪比要求
#define LS_MAX_DI_DT 60000.0f
#define LS_RANGE_MIN 20e-6f
#define LS_RANGE_MAX 300e-6f

// 角度偏移 (开环强励磁)
#define THETA_TIMEOUT_TICKS MS_TO_TICK(1000) // 整定超时时间 (1000ms)
#define THETA_VOLT_AMP 0.6f                  // 强励磁电压幅值 (V)
#define THETA_DELTA_MAX 0.05f                // 静止判断阈值 (°)
#define THETA_STEADY_WIN MS_TO_TICK(100)     // 静止等待时间 (100ms)

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
    g_Param.theta_offset = motor_params.theta_offset;
    g_Param.theta_elec_offset = motor_params.theta_elec_need_180;
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

    // 标记未整定
    motor_params.Rs_valid = false;
    motor_params.L_valid = false;
    motor_params.offset_valid = false;
    motor_params.pole_valid = false;
    motor_params.psi_valid = false;
    motor_params.mech_valid = false;
}

void fMotorParamTune_Reset()
{
    g_tune_ctx.fault = TUNE_FAULT_NONE;
    g_tune_ctx.state = TUNE_STATE_INIT;
}
/* ================================= 电阻整定 (开环 + 滤波 + 差分) ================================= */

static bool _tune_Rs(float i_alpha, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;

    if (ctx->rs_ctx.tick_step++ < RS_FREQ_F)
        return false; // 跳过不整定
    ctx->rs_ctx.tick_step = 0;
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
    else
    { // 跟踪峰值电流和峰值周期 非反转周期

        if (ctx->temp_flag[0])
        {                          // 注入极性为正 电流正向增加
            if (ctx->temp_flag[1]) // 是否继续跟随峰值
            {
                ctx->ls_ctx.ts_cnt[0]++;
                if (i_alpha - ctx->ls_ctx.ipeak[0] < LS_I_STEP_MIN)
                    ctx->temp_flag[1] = false;
                ctx->ls_ctx.ipeak[0] = i_alpha;
            }
            if (ctx->temp_flag[2])
            {
                ctx->ls_ctx.ts_cnt[1]++;
                if (i_beta - ctx->ls_ctx.ipeak[1] < LS_I_STEP_MIN)
                    ctx->temp_flag[2] = false;
                ctx->ls_ctx.ipeak[1] = i_beta;
            }
        }
        else
        {                          // 注入极性为负 电流反向增加
            if (ctx->temp_flag[1]) // 是否继续跟随峰值
            {
                ctx->ls_ctx.ts_cnt[0]++;
                if (i_alpha - ctx->ls_ctx.ipeak[0] > -LS_I_STEP_MIN)
                    ctx->temp_flag[1] = false;
                ctx->ls_ctx.ipeak[0] = i_alpha;
            }
            if (ctx->temp_flag[2])
            {
                ctx->ls_ctx.ts_cnt[1]++;
                if (i_beta - ctx->ls_ctx.ipeak[1] > -LS_I_STEP_MIN)
                    ctx->temp_flag[2] = false;
                ctx->ls_ctx.ipeak[1] = i_beta;
            }
        }
    }
    // 施加电压
    float v_cmd = ctx->temp_flag[0] ? ctx->ls_ctx.v_inj : -ctx->ls_ctx.v_inj;
    fFOC_SetUalphaBeta(v_cmd, v_cmd);

    // 记录初始电流 和 终止电流
    if (ctx->ls_ctx.inject_cnt == 0)
    {

        float di_alpha = ctx->ls_ctx.ipeak[0] - ctx->temp_val[0];
        float di_beta = ctx->ls_ctx.ipeak[1] - ctx->temp_val[1];
        float di_dt_alpha = di_alpha / (ctx->ls_ctx.ts_cnt[0] * params->dt);
        float di_dt_beta = di_beta / (ctx->ls_ctx.ts_cnt[1] * params->dt);

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
                ctx->ls_ctx.i_sum[0][half] += ctx->ls_ctx.ipeak[0];
            }
            if (FABSF(di_dt_beta) > LS_MIN_DI_DT &&
                FABSF(di_dt_beta) < LS_MAX_DI_DT)
            {
                ctx->ls_ctx.di_dt_sum[1][half] += di_dt_beta;
                ctx->ls_ctx.cnt[1][half]++;
                ctx->ls_ctx.i_sum[1][half] += ctx->ls_ctx.ipeak[1];
            }
        }
        ctx->temp_val[0] = i_alpha;
        ctx->temp_val[1] = i_beta;

        // 复位峰值跟踪
        ctx->temp_flag[1] = true;
        ctx->temp_flag[2] = true;
        ctx->ls_ctx.ipeak[0] = i_alpha;
        ctx->ls_ctx.ipeak[1] = i_beta;
        ctx->ls_ctx.ts_cnt[0] = 0;
        ctx->ls_ctx.ts_cnt[1] = 0;
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

bool _tune_ThetaOffset(float theta_mech, float pos, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;

    // 步骤0：α轴定位（施加 Uα 电压）
    if (ctx->theta_ctx.hist_idx == 0)
    {
        // 稳态检测：机械角速度接近0
        float theta_delta = fNormalizeAngle_180(theta_mech - ctx->temp_val[0]);
        if (FABSF(theta_delta) < THETA_DELTA_MAX)
        {
            ctx->theta_ctx.valid_cnt++;
            if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN)
            {
                ctx->theta_ctx.theta_sum += theta_mech;
                if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 2 - 1)
                {
                    ctx->theta_ctx.steady_flag = true;
                }
            }
        }
        else
        {
            ctx->theta_ctx.valid_cnt = 0;
            ctx->theta_ctx.theta_sum = 0;
        }
        ctx->temp_val[0] = theta_mech;

        // α轴定位完成
        if (ctx->theta_ctx.steady_flag)
        {
            float offset = ctx->theta_ctx.theta_sum / THETA_STEADY_WIN;
            offset = fNormalizeAngle_180(offset);
            ctx->theta_ctx.offset_hist[0] = offset; // 保存α轴偏移

            ctx->theta_ctx.offset_hist[1] = pos; // 保存alpha轴位置

            // 重置状态，准备β轴定位
            ctx->theta_ctx.steady_flag = false;
            ctx->theta_ctx.valid_cnt = 0;
            ctx->theta_ctx.theta_sum = 0;
            ctx->temp_val[0] = theta_mech;
            ctx->theta_ctx.hist_idx = 1;

            fFOC_SetUalphaBeta(0, THETA_VOLT_AMP);
        }
        return false;
    }
    // 步骤1：β轴定位（施加 Uβ 电压）
    else if (ctx->theta_ctx.hist_idx == 1)
    {
        // 稳态检测
        float theta_delta = fNormalizeAngle_180(theta_mech - ctx->temp_val[0]);
        if (FABSF(theta_delta) < THETA_DELTA_MAX)
        {
            ctx->theta_ctx.valid_cnt++;
            if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN)
            {
                if (ctx->theta_ctx.valid_cnt >= THETA_STEADY_WIN * 2 - 1)
                {
                    ctx->theta_ctx.steady_flag = true;
                }
            }
        }
        else
        {
            ctx->theta_ctx.valid_cnt = 0;
            ctx->theta_ctx.theta_sum = 0;
        }
        ctx->temp_val[0] = theta_mech;

        // β轴定位完成
        if (ctx->theta_ctx.steady_flag)
        {
            ctx->theta_ctx.offset_hist[2] = pos; // 保存beta轴位置

            // 计算机械角度差（β - α）
            float diff_mech = fNormalizeAngle_180(ctx->theta_ctx.offset_hist[2] - ctx->theta_ctx.offset_hist[1]);

            // 判断转动方向：正差为正向转动，负差为反向转动
            if (diff_mech > 0)
            {
                motor_params.theta_elec_need_180 = false; // 正向，不需要加180°
            }
            else
            {
                motor_params.theta_elec_need_180 = true;
            }
            params->theta_offset = ctx->theta_ctx.offset_hist[0];
            params->offset_valid = true;
            ctx->theta_ctx.hist_idx = 0;

            return true;
        }
        return false;
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
        if (ctx->tick_count++ < TUNE_WAIT_TICKS)
            break;
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
        if (_tune_Ls(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha_im, foc_val.Ibeta_im, params))
        {
            // 电阻上下文复位
            ctx->rs_ctx.step_ticks = 0;

            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
            // todo:这里对电流环PI进行调节

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
        if (_tune_ThetaOffset(foc_val.theta_mech, foc_val.pos_fb, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }

            fSetThetaOffset(motor_params.theta_offset, motor_params.theta_elec_need_180); // 应用于目前计算

            // 角度偏移完成：初始化极对数上下文
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