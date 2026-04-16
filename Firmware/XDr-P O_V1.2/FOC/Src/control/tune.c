#include "tune.h"
#include "math_fast.h"
#include "drive_parameters.h"
#include "encoder.h"
#include "parameter_manager.h"
#include "filter.h"
/* ================================= 全局变量定义 ================================= */
tMotorParams motor_params = {0};
tTuneContext g_tune_ctx = {0};

/* ================================= 辅助函数 ================================= */

static void _fit_from_sums(float sum_x, float sum_y, float sum_xy, float sum_xx, u16 n,
                           float *k, float *b, float *mse)
{
    if (n < 10)
    {
        *k = 0;
        *b = 0;
        *mse = 1e9f;
        return;
    }

    float denom = n * sum_xx - sum_x * sum_x;
    if (fabsf(denom) < 1e-6f)
    {
        *k = 0;
        *b = 0;
        *mse = 1e9f;
        return;
    }

    *k = (n * sum_xy - sum_x * sum_y) / denom;
    *b = (sum_y - (*k) * sum_x) / n;

    // 估算均方误差（简化版，避免存储残差）
    // mse ≈ (Σy² - 2k·Σxy - 2b·Σy + k²·Σxx + 2kb·Σx + b²·n) / n
    // 为简化，此处返回 0，实际调试时可补充 Σy² 累加
    *mse = 0.0f;
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
    g_Param.forward_dir = motor_params.direction;
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
    motor_params.encoder_valid = false;
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

    if (ctx->freq_tick++ < RS_FREQ_F)
        return false; // 跳过不整定
    ctx->freq_tick = 0;
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
        if (ctx->steady_tick >= RS_STEADY_TICKS)
        {
            // === 4. 数据采集 ===
            if (ctx->rs_ctx.step == 0)
            {
                // 记录第一点
                ctx->rs_ctx.i_meas[0] = i_alpha;
                ctx->rs_ctx.v_meas[0] = v_out; // 记录实际输出电压

                // 切换到第二点
                ctx->rs_ctx.step = 1;
                ctx->steady_tick = 0;
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
        ctx->steady_tick = 0;
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
            ctx->steady_tick = 0;
        }
        else if (i_meas > LS_I_LIMIT * 0.8f && ctx->ls_ctx.v_inj > LS_V_START + 0.1f)
        {
            ctx->ls_ctx.v_inj -= 0.1f;
            ctx->steady_tick = 0;
        }
        else
        {
            if (ctx->steady_tick > LS_V_HOLD_MAX_TICKS)
            {
                ctx->ls_ctx.ready = true;
                ctx->steady_tick = 0;
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
    if (ctx->steady_tick >= 4000)
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

// 编码器校准
bool _tune_encoder(float theta_m, tMotorParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    // 1. 频率分频控制
    if (ctx->freq_tick++ < EC_FREQ_F)
        return false; // 跳过不整定
    ctx->freq_tick = 0;

    // 临时变量声明
    float v_alpha = 0, v_beta = 0;
    float theta_step = 0;
    bool finish = false;

    // 2. 状态机跳转
    switch (ctx->encoder_ctx.step)
    {

    // === STATE 0: 直流预对齐 (Align)===
    case 0:
        // 施加 alpha 轴电压，将转子拉至电气 0 度
        ctx->encoder_ctx.v_out = EC_OPEN_LOOP_UQ_MIN;
        fInvParkTransform(ctx->encoder_ctx.v_out, 0, 0, &v_alpha, &v_beta);
        fFOC_SetUalphaBeta(v_alpha, v_beta);

        if (ctx->steady_tick >= EC_ALIGN_ms)
        { // 对齐 300ms
            ctx->steady_tick = 0;
            // 记录起始点
            ctx->encoder_ctx.theta_m_unwrap = theta_m + 360.0f * fGetEncoderNumTurns();
            ctx->encoder_ctx.theta_m_start = ctx->encoder_ctx.theta_m_unwrap;
            ctx->encoder_ctx.theta_e_acc = 0.0f;
            ctx->encoder_ctx.theta_e_raw = 0.0f;
            if (!ctx->encoder_ctx.forward_done)
                ctx->encoder_ctx.step = 1; // 进入正向扫描
            else if (!ctx->encoder_ctx.backward_done)
                ctx->encoder_ctx.step = 2; // 进入反向扫描
            else
                ctx->encoder_ctx.step = 3; // 进入方向校准
        }
        break;
    // === STATE 1/2: 开环扫描 (正向/反向)  ===
    case 1: // Forward
    case 2: // Reverse
    {
        // 2.1 计算电角度增量
        theta_step = EC_OPEN_LOOP_OMEGA * Tcon * EC_FREQ_F;
        if (ctx->encoder_ctx.step == 2)
            theta_step = -theta_step; // 反向取负
        ctx->encoder_ctx.theta_e_acc += theta_step;
        ctx->encoder_ctx.theta_elec = fNormalizeAngle_0_360(ctx->encoder_ctx.theta_e_acc); // 仅用于三角变换

        // 2.2 施加开环电压 (Id=0, Iq=v_out)
        fInvParkTransform(0.0f, ctx->encoder_ctx.v_out, ctx->encoder_ctx.theta_elec, &v_alpha, &v_beta);
        fFOC_SetUalphaBeta(v_alpha, v_beta);

        ctx->encoder_ctx.theta_m_unwrap = theta_m + 360.0f * fGetEncoderNumTurns();

        // 2.4 采样存储 累加最小二乘所需量
        if (FABSF(ctx->encoder_ctx.theta_e_acc - ctx->encoder_ctx.theta_e_raw) >= 10.0f)
        {
            ctx->encoder_ctx.theta_e_raw = ctx->encoder_ctx.theta_e_acc;

            ctx->encoder_ctx.sum_m[ctx->encoder_ctx.step - 1] += ctx->encoder_ctx.theta_m_unwrap;
            ctx->encoder_ctx.sum_e[ctx->encoder_ctx.step - 1] += ctx->encoder_ctx.theta_e_acc;
            ctx->encoder_ctx.sum_me[ctx->encoder_ctx.step - 1] += ctx->encoder_ctx.theta_m_unwrap * ctx->encoder_ctx.theta_e_acc;
            ctx->encoder_ctx.sum_mm[ctx->encoder_ctx.step - 1] += ctx->encoder_ctx.theta_m_unwrap * ctx->encoder_ctx.theta_m_unwrap;
            ctx->encoder_ctx.cnt[ctx->encoder_ctx.step - 1]++;
        }

        // 2.5 扫描结束判断
        if (FABSF(ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start) > 361.0f)
        {
            // 执行拟合
            if (ctx->encoder_ctx.step == 1)
            {                              // 正向完成，保存结果并启动反向
                ctx->encoder_ctx.step = 0; // 切到重新对齐0度
                ctx->encoder_ctx.forward_done = true;
                ctx->steady_tick = 0;
            }
            else
            {                              // 反向完成，进入计算
                ctx->encoder_ctx.step = 0; // 切拟合计算
                ctx->encoder_ctx.backward_done = true;
                ctx->steady_tick = 0;
            }
        }
        break;
    }
        // === STATE 3: 方向校准 ===
    case 3:
    {
        // 施加 beta 轴电压，将转子拉至电气 90度
        ctx->encoder_ctx.v_out = EC_OPEN_LOOP_UQ_MIN;
        fInvParkTransform(0, ctx->encoder_ctx.v_out, 0.0f, &v_alpha, &v_beta);
        fFOC_SetUalphaBeta(v_alpha, v_beta);

        if (ctx->steady_tick >= EC_ALIGN_ms)
        { // 对齐 300ms
            ctx->steady_tick = 0;
            // 记录起始点
            ctx->encoder_ctx.theta_m_unwrap = theta_m + 360.0f * fGetEncoderNumTurns();
            if (ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start > 5.0f)
                motor_params.theta_elec_need_180 = false;
            else if (ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start < -5.0f)
                motor_params.theta_elec_need_180 = true;
            else
            {
                params->encoder_valid = false;
                ctx->fault = TUNE_FAULT_PARAM_INVALID;
                ctx->state = TUNE_STATE_FAULT;
                ctx->encoder_ctx.step = 0; // 失败复位
                return true;
            }
            ctx->encoder_ctx.step = 4; // 进入拟合
        }
        break;
    }
        // === STATE 4: 参数解算 (Fit) ===
    case 4:
    {
        _fit_from_sums(ctx->encoder_ctx.sum_m[0], ctx->encoder_ctx.sum_e[0],
                       ctx->encoder_ctx.sum_me[0], ctx->encoder_ctx.sum_mm[0],
                       ctx->encoder_ctx.cnt[0], &ctx->encoder_ctx.k[0],
                       &ctx->encoder_ctx.b[0], &ctx->encoder_ctx.err[0]);
        _fit_from_sums(ctx->encoder_ctx.sum_m[1], ctx->encoder_ctx.sum_e[1],
                       ctx->encoder_ctx.sum_me[1], ctx->encoder_ctx.sum_mm[1],
                       ctx->encoder_ctx.cnt[1], &ctx->encoder_ctx.k[1],
                       &ctx->encoder_ctx.b[1], &ctx->encoder_ctx.err[1]);
        // 4.1 误差校验
        if (ctx->encoder_ctx.err[0] > EC_FIT_MAX_ERROR || ctx->encoder_ctx.err[1] > EC_FIT_MAX_ERROR)
        {
            params->encoder_valid = false;
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            ctx->state = TUNE_STATE_FAULT;
            ctx->encoder_ctx.step = 0; // 失败复位
            return true;
        }

        // 4.2 计算极对数 (斜率绝对值平均 -> 四舍五入)
        float p_est = (fabsf(ctx->encoder_ctx.k[0]) + fabsf(ctx->encoder_ctx.k[1])) * 0.5f;
        params->pole_pairs = (u8)(p_est + 0.5f);

        // 4.3 极对数范围校验
        if (params->pole_pairs < EC_MIN_POLE_PAIRS || params->pole_pairs > EC_MAX_POLE_PAIRS)
        {
            params->encoder_valid = false;
            ctx->fault = TUNE_FAULT_PARAM_INVALID;
            ctx->state = TUNE_STATE_FAULT;
            return true;
        }

        // 4.4 计算方向 (斜率符号)
        params->direction = (ctx->encoder_ctx.k[0] > 0) ? true : false;

        // 4.5 计算零位偏移 (截距平均，抵消负载角)
        params->theta_offset = fNormalizeAngle_0_360((ctx->encoder_ctx.b[0] + ctx->encoder_ctx.b[1]) * 0.5f);

        ctx->encoder_ctx.step = 5; // 进入完成态
        break;
    }

    // === STATE 5: 完成 (Done) ===
    case 5:
        fFOC_SetUalphaBeta(0, 0); // 停机
        params->encoder_valid = true;
        return true; // 返回 true 表示流程结束
    }

    return false; // 返回 false 表示流程未结束，需下次继续调用
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
    ctx->steady_tick++; // 作为全局稳态计时器
    switch (ctx->state)
    {
    case TUNE_STATE_INIT:
        fMotorParamTune_Init();
        ctx->steady_tick = 0;
        ctx->state = TUNE_STATE_IDLE;
        break;
    case TUNE_STATE_IDLE:
        if (ctx->steady_tick < TUNE_WAIT_TICKS)
            break; // 先静止等待参数稳定
        fFOC_SetSensorMode(ENCODER_CONTROL);
        fFOC_SetRunMode(OPEN_LOOP);
        // 准备工作：初始化电阻整定上下文
        ctx->rs_ctx.i_target = RS_I_TARGET_1;
        ctx->rs_ctx.v_cmd = 0;
        ctx->rs_ctx.step = 0;
        ctx->temp_val[0] = 0;
        ctx->temp_val[1] = 0;
        ctx->steady_tick = 0;
        ctx->timeout_tick = 0;

        ctx->state = TUNE_STATE_RS;
        break;

    case TUNE_STATE_RS:
        // 超时检测
        //        if (ctx->timeout_tick++ > RS_TIMEOUT_TICKS)
        //        {
        //            ctx->state = TUNE_STATE_FAULT;
        //            ctx->fault = TUNE_FAULT_TIMEOUT;
        //        }
        // 校准中
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
            ctx->steady_tick = 0;
            ctx->timeout_tick = 0;
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

            // 电感完成：初始化编码器校准上下文
            ctx->encoder_ctx.theta_elec = 0;
            ctx->encoder_ctx.v_out = 0;
            ctx->encoder_ctx.forward_done = false;
            ctx->encoder_ctx.backward_done = false;
            ctx->encoder_ctx.step = 0;
            ctx->steady_tick = 0;

            fFOC_SetUalphaBeta(0, 0);

            ctx->timeout_tick = 0;
            ctx->state = TUNE_STATE_ENCODER;
        }
        break;

    case TUNE_STATE_ENCODER:
        if (_tune_encoder(foc_val.theta_mech, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_STATE_FAULT;
                break;
            }
						//todo:写入参数
//            fSetThetaOffset(motor_params.theta_offset, motor_params.theta_elec_need_180); // 应用于目前计算

            //  极对数完成：初始化磁链上下文
            ctx->psi_ctx.sum_e_mag = 0;
            ctx->psi_ctx.sum_omega = 0;
            ctx->psi_ctx.valid_cnt = 0;
            ctx->psi_ctx.ready = false;
            ctx->steady_tick = 0;

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
            ctx->steady_tick = 0;

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