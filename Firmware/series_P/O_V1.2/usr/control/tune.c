#include "tune.h"
#include "math_fast.h"
#include "bsp_adc.h"

#include "device.h"
#include "parameter_manager.h"
#include "filter.h"
/* ================================= 全局变量定义 ================================= */
tTuneParams temp_params = {0};
tTuneContext g_tune_ctx = {0};

tFirstOrderLagFilter rs_i_filter;
/* ================================= 辅助函数 ================================= */
// sum型线性拟合
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
// 控制带宽、滤波系数与PID参数的经验公式
void fCalculateControlParams()
{
    // todo:这里对电流环PI进行调节
    float fn_d = 1 / (MATH_2PI * temp_params.Ld / temp_params.Rs);
    float wc_d = MATH_2PI * 0.5f * (2 * fn_d < F_CURRENT / 10 ? 2 * fn_d : F_CURRENT / 10);
    temp_params.id_kp = wc_d * temp_params.Ld;
    temp_params.id_ki = wc_d * temp_params.Rs;

    float fn_q = 1 / (MATH_2PI * temp_params.Lq / temp_params.Rs);
    float wc_q = MATH_2PI * 0.6f * (2 * fn_q < F_CURRENT / 10 ? 2 * fn_q : F_CURRENT / 10);
    temp_params.iq_Kp = wc_q * temp_params.Lq;
    temp_params.iq_Ki = wc_q * temp_params.Rs;

    // 取电流环开环截止频率 (Hz)
    float f_c_open = fminf(wc_d, wc_q) / MATH_2PI;

    // 设计滤波截止频率：取开环截止频率的 4倍
    float f_filter = 4.0f * f_c_open;

    // 上下限约束
    float f_sw = (float)F_PWM;     // ADC采样频率 = PWM频率
    float f_max = f_sw / 8.0f;     // 最大允许滤波截止频率（保证滤除开关纹波）
    float f_min = 2.0f * f_c_open; // 最小允许值（避免影响环路）

    if (f_filter > f_max)
        f_filter = f_max;
    if (f_filter < f_min)
        f_filter = f_min;

    // 计算一阶低通滤波系数 alpha
    temp_params.cur_fiter_alpha = 1.0f - expf(-MATH_2PI * f_filter / f_sw);
}

/* ================================= 参数访问实现 ================================= */
void fMotorParamTune_ForceSave(void)
{

    // 过程中会直接写入motor用于后续控制
    //  这里是集中写入参数flash中转站
    g_Param.motor_rs = temp_params.Rs;
    g_Param.motor_ld = temp_params.Ld;
    g_Param.motor_lq = temp_params.Lq;
    g_Param.motor_psif = temp_params.Psi_f;
    g_Param.motor_ke = temp_params.Ke;
    g_Param.motor_j = temp_params.J;
    g_Param.motor_b = temp_params.B;
    g_Param.motor_polepairs = temp_params.pole_pairs;
    g_Param.theta_offset = temp_params.theta_offset;
    g_Param.theta_elec_offset = temp_params.theta_elec_need_180;
    g_Param.forward_dir = temp_params.direction;

    g_Param.kp_current = temp_params.iq_Kp;
    g_Param.ki_current = temp_params.iq_Ki;
    g_Param.kp_weakmag = temp_params.id_kp;
    g_Param.ki_weakmag = temp_params.id_ki;

    g_Param.cur_fiter_alpha = temp_params.cur_fiter_alpha;
    g_Param.adc_U_zero_offset = temp_params.uadc_offset;
    g_Param.adc_V_zero_offset = temp_params.vadc_offset;
    g_Param.adc_W_zero_offset = temp_params.wadc_offset;
}

/* ================================= 初始化与重置 ================================= */
// todo:后续添加无感整定，根据选择的感应模式校准，无感只校准电机，有感校准电机和编码器，直接校准电机，直接用HFI、SMO做精准的控制
void fMotorParamTune_Init()
{

    memset(&g_tune_ctx, 0, sizeof(tTuneContext));
    memset(&temp_params, 0, sizeof(tTuneParams));
    temp_params.KV = g_Param.motor_kv;
    temp_params.dt = T_CON;
    fFirstOrderLagInit(&rs_i_filter, 0.04f, 0);
}

void fMotorParamTune_Reset()
{
    g_tune_ctx.fault = TUNE_FAULT_NONE;
    g_tune_ctx.state = TUNE_INIT;
}
/* ================================= 电阻整定 (开环 + 滤波 + 差分) ================================= */

static bool _tune_Rs(float i_alpha, tTuneParams *params)
{
    tTuneContext *ctx = &g_tune_ctx;
    float i_a = fFirstOrderLagFilter(&rs_i_filter, i_alpha);

    if (ctx->freq_tick++ < RS_FREQ_F)
        return false; // 跳过不整定
    ctx->freq_tick = 0;
    //===滞环电流控制===
    float err = ctx->rs_ctx.i_target - i_a;
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
    float v_dead_comp = (i_a > 0) ? RS_DEADTIME_VCOMP : -RS_DEADTIME_VCOMP;
    float v_out = CLAMP(ctx->rs_ctx.v_cmd + v_dead_comp, -RS_V_LIMIT, RS_V_LIMIT);

    // 稳态判断
    float i_err = FABSF(i_a - ctx->rs_ctx.i_target);
    if (i_err < RS_STEADY_ERR_THR)
    {
        if (ctx->steady_tick >= RS_STEADY_TICKS)
        {
            // === 4. 数据采集 ===
            if (ctx->rs_ctx.step == 0)
            {
                // 记录第一点
                ctx->rs_ctx.i_meas[0] = i_a;
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
                ctx->rs_ctx.i_meas[1] = i_a;
                ctx->rs_ctx.v_meas[1] = v_out;

                // === 5. 差分计算电阻 ===
                float delta_i = ctx->rs_ctx.i_meas[1] - ctx->rs_ctx.i_meas[0];
                float delta_v = ctx->rs_ctx.v_meas[1] - ctx->rs_ctx.v_meas[0];

                // 信噪比检查
                if (FABSF(delta_i) < RS_MIN_DELTA_I)
                {
                    ctx->fault = TUNE_FAULT_CURRENT_VIBRATION;
                    return true;
                }

                // 计算电阻
                params->Rs = delta_v / (delta_i + 1e-6f);

                // 合理性校验
                if (params->Rs < RS_RANGE_MIN || params->Rs > RS_RANGE_MAX)
                {
                    ctx->fault = TUNE_FAULT_RSLS_INVALID;
                    return true;
                }

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

/**
 * @brief 累加当前采样点对DFT的贡献（无缓冲区）
 * @param i_alpha    α轴电流
 * @param i_beta     β轴电流
 * @param omega_dt   ω_inj * dt (rad) 每次固定角度增量
 * @param sum_re     实部累加器指针
 * @param sum_im     虚部累加器指针
 * @param cnt        当前周期内已采样点数指针（从0开始递增）
 */
static void dft_accumulate(float i_alpha, float i_beta, float omega_dt,
                           float *sum_re, float *sum_im, uint16_t *cnt)
{
    float angle = omega_dt * (*cnt); // θ = ωt
    float c = cosf(angle);
    float s = -sinf(angle); // e^{-jθ}
    // 复数乘法: (i_alpha + j i_beta) * (c + j s) 的实部和虚部
    // 注意：实际需要的 DFT 系数是 e^{-jθ}，即 (c + j s) 其中 s = -sin(θ)
    *sum_re += i_alpha * c - i_beta * s;
    *sum_im += i_alpha * s + i_beta * c;
    (*cnt)++;
}
/**
 * @brief 根据累加和计算电流幅值
 * @param sum_re   实部累加和
 * @param sum_im   虚部累加和
 * @param samples  该周期内的采样点数（恒为 SAMPLES_PER_CYCLE）
 * @return 电流基波幅值 (A)
 */
static float dft_get_amplitude(float sum_re, float sum_im, uint16_t samples)
{
    float scale = 2.0f / samples;
    float re = sum_re * scale;
    float im = sum_im * scale;
    return sqrtf(re * re + im * im);
}
static void dft_reset_accumulator(float *sum_re, float *sum_im, uint16_t *cnt)
{
    *sum_re = 0.0f;
    *sum_im = 0.0f;
    *cnt = 0;
}

/* ================================= 电感整定（alpha beta轴方波注入） ================================= */
static bool
_tune_Ls(float v_alpha, float v_beta, float i_alpha, float i_beta, tTuneParams *params)
{

    tTuneContext *ctx = &g_tune_ctx;
    const float omega_inj = MATH_2PI * LS_INJECT_FREQ_HZ; // 注入角频率 (°/s)
    const float omega_dt = omega_inj * params->dt;        // 每步相位增量 (°)

    static float inj_angle = 0.0f; // 当前注入电压相位
    float v_alpha_cmd, v_beta_cmd;

    // 为了代码可读性，将上下文指针暂存
    float *sum_re = &ctx->ls_ctx.sum_re;
    float *sum_im = &ctx->ls_ctx.sum_im;
    uint16_t *sample_cnt = &ctx->ls_ctx.sample_cnt;

    switch (ctx->ls_ctx.state)
    {
    // ==================== 状态0：自适应注入电压 ====================
    case 0:
        // 初始化自适应过程
        if (!ctx->ls_ctx.ready)
        {
            ctx->ls_ctx.v_inj = LS_V_START;
            ctx->ls_ctx.i_target = LS_I_TARGET;
            dft_reset_accumulator(sum_re, sum_im, sample_cnt);
            ctx->ls_ctx.amp_sum = 0.0f;
            ctx->ls_ctx.cycle_cnt = 0;
            inj_angle = 0.0f;
            ctx->ls_ctx.ready = true;
        }

        // 生成旋转电压 (αβ 旋转，使电流幅值与转子位置无关)

        v_alpha_cmd = ctx->ls_ctx.v_inj * arm_cos_f32(inj_angle);
        v_beta_cmd = ctx->ls_ctx.v_inj * arm_sin_f32(inj_angle);
        fFOC_SetUalphaBeta(v_alpha_cmd, v_beta_cmd);

        // 更新注入相位
        inj_angle += omega_dt;
        if (inj_angle > MATH_2PI)
            inj_angle -= MATH_2PI;

        // DFT 累加当前的电流值
        dft_accumulate(i_alpha, i_beta, omega_dt, sum_re, sum_im, sample_cnt);

        // 完成一个注入周期
        if (*sample_cnt >= LS_INJECT_FREQ_TICK)
        {
            // 计算这个周期的电流幅值
            float I_mag = dft_get_amplitude(*sum_re, *sum_im, LS_INJECT_FREQ_TICK);
            ctx->ls_ctx.amp_sum += I_mag;
            ctx->ls_ctx.cycle_cnt++;

            // 重置累加器，准备下一个周期
            dft_reset_accumulator(sum_re, sum_im, sample_cnt);

            if (ctx->ls_ctx.cycle_cnt >= DFT_AVG_CYCLES)
            {
                float I_avg = ctx->ls_ctx.amp_sum / ctx->ls_ctx.cycle_cnt;
                ctx->ls_ctx.i_meas = I_avg;

                // 判断电流是否在目标范围 [I_TARGET - HYST, I_TARGET + HYST]
                if (I_avg < LS_I_TARGET * 0.9f && ctx->ls_ctx.v_inj < LS_V_MAX - 0.1f)
                {
                    ctx->ls_ctx.v_inj += 0.2f; // 增加电压
                    if (ctx->ls_ctx.v_inj > LS_V_MAX)
                        ctx->ls_ctx.v_inj = LS_V_MAX;
                }
                else if (I_avg > LS_I_TARGET * 1.1f && ctx->ls_ctx.v_inj > LS_V_START + 0.1f)
                {
                    ctx->ls_ctx.v_inj -= 0.2f; // 降低电压
                    if (ctx->ls_ctx.v_inj < LS_V_START)
                        ctx->ls_ctx.v_inj = LS_V_START;
                }
                else
                {
                    // 电压已合适，结束自适应阶段
                    ctx->ls_ctx.state = 1; // 进入转子预定位
                    ctx->steady_tick = 0;  // 重置定时器
                    ctx->ls_ctx.cycle_cnt = 0;
                    fFOC_SetUalphaBeta(0, 0); // 先关断电压
                    return false;
                }

                // 重置平均累加器，继续下一轮自适应
                ctx->ls_ctx.amp_sum = 0.0f;
                ctx->ls_ctx.cycle_cnt = 0;
            }
        }
        return false; // 尚未完成

    // ==================== 状态1：转子预定位（拉至0°电角度） ====================
    case 1:
        if (ctx->steady_tick > ALIGN_TIME_MS)
        {
            // 对齐完成，断电，等待电流衰减
            fFOC_SetUalphaBeta(0, 0);
            if (ctx->steady_tick > ALIGN_TIME_MS * 2)
            {
                ctx->ls_ctx.state = 2;
                ctx->steady_tick = 0;

                // 初始化 DFT 测量 Ld

                dft_reset_accumulator(sum_re, sum_im, sample_cnt);
                ctx->ls_ctx.amp_sum = 0.0f;
                ctx->ls_ctx.cycle_cnt = 0;
                inj_angle = 0.0f;
            }
            return false;
        }
        // 施加直流电压使转子对齐到α轴（电角度0°）
        fFOC_SetUalphaBeta(ctx->ls_ctx.v_inj, 0.0f);

        return false;

    // ==================== 状态2：测量 Ld ====================
    case 2:

        // 注入正弦电压（仅α轴，因转子已在0°，α轴对应d轴）

        v_alpha_cmd = ctx->ls_ctx.v_inj * arm_cos_f32(inj_angle);
        v_beta_cmd = 0.0f;
        fFOC_SetUalphaBeta(v_alpha_cmd, v_beta_cmd);

        inj_angle += omega_dt;
        if (inj_angle > MATH_2PI)
            inj_angle -= MATH_2PI;

        dft_accumulate(i_alpha, i_beta, omega_dt, sum_re, sum_im, sample_cnt);

        if (*sample_cnt >= LS_INJECT_FREQ_TICK)
        {
            float I_mag = dft_get_amplitude(*sum_re, *sum_im, LS_INJECT_FREQ_TICK);
            ctx->ls_ctx.amp_sum += I_mag;
            ctx->ls_ctx.cycle_cnt++;
            dft_reset_accumulator(sum_re, sum_im, sample_cnt);

            if (ctx->ls_ctx.cycle_cnt >= DFT_AVG_CYCLES)
            {
                float I_avg = ctx->ls_ctx.amp_sum / ctx->ls_ctx.cycle_cnt;
                // 电阻补偿：L = sqrt( (V/R)^2? 正确公式: L = sqrt( (V/I)^2 - Rs^2 ) / ω
                float V_rms = ctx->ls_ctx.v_inj * MATH_1_SQRT2; // 注入电压峰值 -> 有效值
                float I_rms = I_avg * MATH_1_SQRT2;
                float Z = V_rms / I_rms; // 阻抗模
                float L = 0.0f;
                if (Z * Z > params->Rs * params->Rs)
                {
                    arm_sqrt_f32(Z * Z - params->Rs * params->Rs, &L);
                    L = L / omega_inj;
                }
                else
                {
                    L = 0.0f; // 不合理，置0
                }
                params->Ld = L;
                // 测量完成，进入状态3：旋转转子至90°
                ctx->ls_ctx.state = 3;
                ctx->steady_tick = 0;
                fFOC_SetUalphaBeta(0, 0);
            }
        }
        return false;

    // ==================== 状态3：旋转转子至90°电角度（准备测Lq） ====================
    case 3:
        if (ctx->steady_tick > ALIGN_TIME_MS)
        {
            // 对齐完成，断电，等待电流衰减
            fFOC_SetUalphaBeta(0, 0);
            if (ctx->steady_tick > ALIGN_TIME_MS * 2)
            {
                ctx->ls_ctx.state = 4;
                ctx->steady_tick = 0;

                // 初始化dft 测量Lq

                dft_reset_accumulator(sum_re, sum_im, sample_cnt);
                ctx->ls_ctx.amp_sum = 0.0f;
                ctx->ls_ctx.cycle_cnt = 0;
                inj_angle = 0.0f;
            }
            return false;
        }
        // 施加β轴直流电压，使转子从0°转到90°电角度
        fFOC_SetUalphaBeta(0.0f, ctx->ls_ctx.v_inj);

        return false;

    // ==================== 状态4：测量 Lq ====================
    case 4:

        // 依旧在α轴注入正弦（此时转子90°，α轴对准q轴）
        v_alpha_cmd = ctx->ls_ctx.v_inj * arm_cos_f32(inj_angle);
        v_beta_cmd = 0.0f;
        fFOC_SetUalphaBeta(v_alpha_cmd, v_beta_cmd);

        inj_angle += omega_dt;
        if (inj_angle > MATH_2PI)
            inj_angle -= MATH_2PI;

        dft_accumulate(i_alpha, i_beta, omega_dt, sum_re, sum_im, sample_cnt);

        if (*sample_cnt >= LS_INJECT_FREQ_TICK)
        {
            float I_mag = dft_get_amplitude(*sum_re, *sum_im, LS_INJECT_FREQ_TICK);
            ctx->ls_ctx.amp_sum += I_mag;
            ctx->ls_ctx.cycle_cnt++;
            dft_reset_accumulator(sum_re, sum_im, sample_cnt);

            if (ctx->ls_ctx.cycle_cnt >= DFT_AVG_CYCLES)
            {
                float I_avg = ctx->ls_ctx.amp_sum / ctx->ls_ctx.cycle_cnt;
                float V_rms = ctx->ls_ctx.v_inj * MATH_1_SQRT2;
                float I_rms = I_avg * MATH_1_SQRT2;
                float Z = V_rms / I_rms;
                float L = 0.0f;
                if (Z * Z > params->Rs * params->Rs)
                {
                    arm_sqrt_f32(Z * Z - params->Rs * params->Rs, &L);
                    L = L / omega_inj;
                }
                params->Lq = L;
                // 测量完成，进入状态5
                ctx->ls_ctx.state = 5;
                fFOC_SetUalphaBeta(0, 0);
            }
        }
        return false;

    // ==================== 状态5：完成，保存结果 ====================
    case 5:
        // 可选：对结果进行合理性检查（例如范围 0.01mH ~ 100mH）
        if (params->Ld < LS_RANGE_MIN || params->Ld > LS_RANGE_MAX)
            ctx->fault = TUNE_FAULT_RSLS_INVALID;
        if (params->Lq < LS_RANGE_MIN || params->Lq > LS_RANGE_MAX)
            ctx->fault = TUNE_FAULT_RSLS_INVALID;
        return true; // 校准完成

    default:
        return true;
    }

    return false; // 未完成
}

// 编码器校准
bool _tune_encoder(float theta_m, tTuneParams *params)
{
    const float THETA_STEP = EC_OPEN_LOOP_OMEGA * T_CON * EC_FREQ_F;
    tTuneContext *ctx = &g_tune_ctx;
    // 1. 频率分频控制
    if (ctx->freq_tick++ < EC_FREQ_F)
        return false; // 跳过不整定
    ctx->freq_tick = 0;

    // 临时变量声明
    float v_alpha = 0, v_beta = 0;

    // 2. 状态机跳转
    switch (ctx->encoder_ctx.step)
    {
        // === STATE 0: 确定要施加的合理电压===
    case 0:
        ctx->encoder_ctx.theta_m_unwrap = theta_m + 360.0f * fGetEncoderNumTurns();
        switch (ctx->encoder_ctx.test_step)
        {
        case 0:
            fInvParkTransform(ctx->encoder_ctx.v_out, 0, 0, &v_alpha, &v_beta);
            fFOC_SetUalphaBeta(v_alpha, v_beta);
            if (ctx->steady_tick >= EC_ALIGN_ms)
            {
                ctx->steady_tick = 0;
                ctx->encoder_ctx.theta_m_start = ctx->encoder_ctx.theta_m_unwrap;
                ctx->encoder_ctx.test_step = 1;
            }
            break;
        case 1:
            fInvParkTransform(ctx->encoder_ctx.v_out, 0, 180.0f, &v_alpha, &v_beta);
            fFOC_SetUalphaBeta(v_alpha, v_beta);
            if (ctx->steady_tick >= EC_ALIGN_ms)
            {
                ctx->steady_tick = 0;
                ctx->encoder_ctx.delta_theta[0] = FABSF(ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start);
                ctx->encoder_ctx.theta_m_start = ctx->encoder_ctx.theta_m_unwrap;
                ctx->encoder_ctx.test_step = 2;
            }
            break;
        case 2:
            fInvParkTransform(ctx->encoder_ctx.v_out, 0, 0.0f, &v_alpha, &v_beta);
            fFOC_SetUalphaBeta(v_alpha, v_beta);
            if (ctx->steady_tick >= EC_ALIGN_ms)
            {
                ctx->steady_tick = 0;
                ctx->encoder_ctx.delta_theta[1] = FABSF(ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start);
                ctx->encoder_ctx.theta_m_start = ctx->encoder_ctx.theta_m_unwrap;
                ctx->encoder_ctx.test_step = 3;
            }
            break;
        case 3:
            if (ctx->encoder_ctx.delta_theta[0] < 10.0f || ctx->encoder_ctx.delta_theta[1] < 10.0f)
            {
                ctx->encoder_ctx.v_out += EC_OPEN_LOOP_UQ_STEP;
                ctx->encoder_ctx.v_out = CLAMP(ctx->encoder_ctx.v_out, EC_OPEN_LOOP_UQ_MIN, EC_OPEN_LOOP_UQ_MAX);
                ctx->encoder_ctx.test_step = 0;
                break;
            }
            ctx->encoder_ctx.test_step = 0;
            ctx->encoder_ctx.step = 1;
            break;
        }
        break;

    // === STATE 1: 直流预对齐 (Align)===
    case 1:
        // 施加 alpha 轴电压，将转子拉至电气 0 度

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
                ctx->encoder_ctx.step = 2; // 进入正向扫描
            else if (!ctx->encoder_ctx.backward_done)
                ctx->encoder_ctx.step = 3; // 进入反向扫描
            else
                ctx->encoder_ctx.step = 4; // 进入方向校准
        }
        break;
    // === STATE 2/3: 开环扫描 (正向/反向)  ===
    case 2: // Forward
    case 3: // Reverse
    {
        // 2.1 计算电角度增量
        if (ctx->encoder_ctx.step == 3)
            ctx->encoder_ctx.theta_e_acc -= THETA_STEP; // 反向取负
        else
            ctx->encoder_ctx.theta_e_acc += THETA_STEP;
        ctx->encoder_ctx.theta_elec = fNormalizeAngle_0_360(ctx->encoder_ctx.theta_e_acc); // 仅用于三角变换

        // 2.2 施加开环电压 对应角度的ud
        fInvParkTransform(ctx->encoder_ctx.v_out, 0.0f, ctx->encoder_ctx.theta_elec, &v_alpha, &v_beta);
        fFOC_SetUalphaBeta(v_alpha, v_beta);

        ctx->encoder_ctx.theta_m_unwrap = theta_m + 360.0f * fGetEncoderNumTurns();

        // 2.4 采样存储 累加最小二乘所需量
        if (FABSF(ctx->encoder_ctx.theta_e_acc - ctx->encoder_ctx.theta_e_raw) >= 10.0f)
        {
            ctx->encoder_ctx.theta_e_raw = ctx->encoder_ctx.theta_e_acc;

            ctx->encoder_ctx.sum_m[ctx->encoder_ctx.step - 2] += ctx->encoder_ctx.theta_m_unwrap;
            ctx->encoder_ctx.sum_e[ctx->encoder_ctx.step - 2] += ctx->encoder_ctx.theta_e_acc;
            ctx->encoder_ctx.sum_me[ctx->encoder_ctx.step - 2] += ctx->encoder_ctx.theta_m_unwrap * ctx->encoder_ctx.theta_e_acc;
            ctx->encoder_ctx.sum_mm[ctx->encoder_ctx.step - 2] += ctx->encoder_ctx.theta_m_unwrap * ctx->encoder_ctx.theta_m_unwrap;
            ctx->encoder_ctx.cnt[ctx->encoder_ctx.step - 2]++;
        }

        // 2.5 扫描结束判断
        if (FABSF(ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start) > 361.0f)
        {
            // 执行拟合
            if (ctx->encoder_ctx.step == 2)
            { // 正向完成，保存结果并启动反向

                ctx->encoder_ctx.forward_done = true;
                ctx->encoder_ctx.step = 1; // 切到重新对齐0度
                ctx->steady_tick = 0;
            }
            else
            { // 反向完成，进入计算

                ctx->encoder_ctx.backward_done = true;
                ctx->encoder_ctx.step = 1; // 切方向校准
                ctx->steady_tick = 0;
            }
        }
        break;
    }
        // === STATE 4: 方向校准 ===
    case 4:
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
                temp_params.theta_elec_need_180 = true;
            else if (ctx->encoder_ctx.theta_m_unwrap - ctx->encoder_ctx.theta_m_start < -5.0f)
                temp_params.theta_elec_need_180 = false;
            else
            {
                ctx->fault = TUNE_FAULT_MECH_LOCKED;
                ctx->state = TUNE_FAILED;
                ctx->encoder_ctx.step = 0; // 失败复位
                return true;
            }
            ctx->encoder_ctx.step = 5; // 进入拟合
        }
        break;
    }
        // === STATE 5: 参数解算 (Fit) ===
    case 5:
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
            ctx->fault = TUNE_FAULT_ENCODER_INVALID;
            ctx->state = TUNE_FAILED;
            ctx->encoder_ctx.step = 0; // 失败复位
            return true;
        }

        // 4.2 计算极对数 (斜率绝对值平均 -> 四舍五入)
        float p_est = (fabsf(ctx->encoder_ctx.k[0]) + fabsf(ctx->encoder_ctx.k[1])) * 0.5f;
        params->pole_pairs = (u8)(p_est + 0.5f);

        // 4.3 极对数范围校验
        if (params->pole_pairs < EC_MIN_POLE_PAIRS || params->pole_pairs > EC_MAX_POLE_PAIRS)
        {

            ctx->fault = TUNE_FAULT_ENCODER_INVALID;
            ctx->state = TUNE_FAILED;
            return true;
        }

        if (params->pole_pairs != g_Param.motor_polepairs)
        {
            ctx->fault = TUNE_FAULT_POLEPAIRS_MISMATCH;
            ctx->state = TUNE_FAILED;
            return true;
        }
        // 4.4 计算方向 (斜率符号)
        params->direction = (ctx->encoder_ctx.k[0] > 0) ? true : false;

        // 4.5 计算零位偏移 (截距平均，抵消负载角)
        float o_est_f = -(ctx->encoder_ctx.b[0] / ctx->encoder_ctx.k[0]);
        float o_est_b = -(ctx->encoder_ctx.b[1] / ctx->encoder_ctx.k[1]);
        params->theta_offset = fNormalizeAngle_0_360((o_est_f + o_est_b) * 0.5f);

        ctx->encoder_ctx.step = 6; // 进入完成态
        break;
    }

    // === STATE 6: 完成 (Done) ===
    case 6:
        fFOC_SetUalphaBeta(0, 0); // 停机
        return true;              // 返回 true 表示流程结束
    }

    return false; // 返回 false 表示流程未结束，需下次继续调用
}

/* ================================= 磁链整定 (SMO 框架) ================================= */
static bool _tune_PsiF(tFOC_val foc_val, tTuneParams *params)
{
    // TODO: SMO 高速整定框架
    // 1. 高速运行 (例如 1000rpm+)，SMO 估算反电动势
    // 2. Ke = |E| / omega_elec
    // 3. Psi_f = Ke / pole_pairs

    // 临时占位：使用 KV 反推（后续替换为 SMO 实测）
    params->Ke = 60.0f / (2.0f * MATH_PI * temp_params.KV * params->pole_pairs);
    params->Psi_f = params->Ke / params->pole_pairs;
    return true;
}

/* ================================= 转动惯量/摩擦系数整定 (框架) ================================= */
static bool _tune_JB(tFOC_val foc_val, tTuneParams *params)
{
    // TODO: 阶跃响应法框架
    // 1. 施加阶跃转矩 (例如 iq=2A)
    // 2. 记录加速度曲线: alpha = d(omega)/dt
    // 3. J = T / alpha (忽略摩擦), B = (T - J*alpha) / omega (匀速段)

    // 临时占位：使用默认值跳过
    params->J = 0.0001f;
    params->B = 0.001f;
    return true;
}

/* ================================= 整定主循环 (状态切换时初始化) ================================= */
eTuneState fMotorParamTune_Update(tFOC_val foc_val)
{
    tTuneContext *ctx = &g_tune_ctx;
    tTuneParams *params = &temp_params;
    ctx->steady_tick++; // 作为全局稳态计时器
    switch (ctx->state)
    {
    case TUNE_INIT:
        fMotorParamTune_Init();
        fFOC_SetSensorMode(ENCODER_CONTROL);
        fFOC_SetRunMode(OPEN_LOOP);
        ctx->steady_tick = 0;
        ctx->state = TUNE_IDLE;
        break;
    case TUNE_IDLE:
        if (ctx->steady_tick < TUNE_WAIT_TICKS)
            break; // 先静止等待参数稳定
        if (false == BSP_AdcCalibrateCurrent(&temp_params.uadc_offset, &temp_params.vadc_offset, &temp_params.wadc_offset))
            break; // 电流校准

        // 准备工作：初始化电阻整定上下文
        ctx->rs_ctx.i_target = RS_I_TARGET_1;
        ctx->rs_ctx.v_cmd = 0;
        ctx->rs_ctx.step = 0;
        ctx->temp_val[0] = 0;
        ctx->temp_val[1] = 0;
        ctx->steady_tick = 0;
        ctx->timeout_tick = 0;

        ctx->state = TUNE_RESISTANCE;
        break;

    case TUNE_RESISTANCE:
        // 超时检测
        //        if (ctx->timeout_tick++ > RS_TIMEOUT_TICKS)
        //        {
        //            ctx->state = TUNE_FAILED;
        //            ctx->fault = TUNE_FAULT_TIMEOUT;
        //        }
        // 校准中
        if (_tune_Rs(foc_val.Ialpha, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_FAILED;
                break;
            }

            // 电阻完成：初始化电感整定上下文
            ctx->ls_ctx.v_inj = LS_V_START;
            ctx->ls_ctx.ready = false;
            ctx->steady_tick = 0;
            ctx->timeout_tick = 0;
            ctx->state = TUNE_INDUCTANCE;
        }
        break;

    case TUNE_INDUCTANCE:
        if (_tune_Ls(foc_val.Ualpha, foc_val.Ubeta, foc_val.Ialpha, foc_val.Ibeta, params))
        {
            // 电阻上下文复位
            ctx->rs_ctx.step_ticks = 0;

            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_FAILED;
                break;
            }

            fCalculateControlParams();
            // 电感完成：初始化编码器校准上下文

            fSetEncoderAngleZero(); // 编码器圈数归零
            ctx->encoder_ctx.theta_elec = 0;
            ctx->encoder_ctx.v_out = 0;
            ctx->encoder_ctx.forward_done = false;
            ctx->encoder_ctx.backward_done = false;
            ctx->encoder_ctx.step = 0;
            ctx->steady_tick = 0;

            fFOC_SetUalphaBeta(0, 0);
            ctx->encoder_ctx.v_out = EC_OPEN_LOOP_UQ_MIN;

            ctx->timeout_tick = 0;
            ctx->state = TUNE_ENCODER;
        }
        break;

    case TUNE_ENCODER:
        if (_tune_encoder(foc_val.theta_mech, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_FAILED;
                break;
            }
            // todo:写入参数
            //            fSetThetaOffset(temp_params.theta_offset, temp_params.theta_elec_need_180); // 应用于目前计算

            //  极对数完成：初始化磁链上下文
            ctx->psi_ctx.sum_e_mag = 0;
            ctx->psi_ctx.sum_omega = 0;
            ctx->psi_ctx.valid_cnt = 0;
            ctx->psi_ctx.ready = false;
            ctx->steady_tick = 0;

            ctx->state = TUNE_ELEC_PARAM;
        }
        break;

    case TUNE_ELEC_PARAM:
        if (_tune_PsiF(foc_val, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_FAILED;
                break;
            }

            //  磁链完成：初始化机械参数上下文
            ctx->jb_ctx.accel_phase = false;
            ctx->jb_ctx.sample_cnt = 0;
            ctx->jb_ctx.sum_torque = 0;
            ctx->jb_ctx.sum_accel = 0;
            ctx->jb_ctx.ready = false;
            ctx->steady_tick = 0;

            ctx->state = TUNE_MECH_PARAM;
        }
        break;

    case TUNE_MECH_PARAM:
        if (_tune_JB(foc_val, params))
        {
            if (ctx->fault != TUNE_FAULT_NONE)
            {
                ctx->state = TUNE_FAILED;
                break;
            }

            // todo:这里可以对速度环PI和位置环PID 参数进行调节
            //  结束：保存参数并进入完成状态

            fMotorParamTune_ForceSave();
            ctx->state = TUNE_DONE;
        }
        break;

    default:
        break;
    }
    return ctx->state;
}

/* ================================= 辅助接口 ================================= */
eTuneFault fMotorParamTune_GetFault(void) { return g_tune_ctx.fault; }