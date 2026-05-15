#include "smo.h"
#include "math_fast.h"
#include "usr_config.h"
#include "filter.h"

#define SMO_DTICK 4
#define SMO_FREQ fpwm / SMO_DTICK

#define CURRENT_ALPHA 0.25f
#define EMF_ALPHA 0.07f
#define OMEGA_ALPHA 0.01f

tFirstOrderLagFilter I_alpha_lpf;
tFirstOrderLagFilter I_beta_lpf;
tFirstOrderLagFilter emf_alpha_lpf;
tFirstOrderLagFilter emf_beta_lpf;

tFirstOrderLagFilter omega_lpf;

tSMO g_smo;

// 默认配置
static const tSMO_Config SMO_DEFAULT_CFG = {
    .k_sl_base = 15.0f,
    .k_sl_min_ratio = 0.5f,
    .omega_adapt_start = 100.0f,
    .omega_adapt_end = 400.0f,
    .delta = 0.1f,
    .min_omega_elec = 50.0f,
    .max_omega_elec = 2000.0f,
    .emf_max = 15.0f};

// 预计算不变量
__STATIC_INLINE void _SmoPrecompute(tSMO *p)
{
    float L_avg = (p->ld + p->lq) * 0.5f;
    p->inv_l_eff = 1.0f / (L_avg + p->rs * p->dt);
}

// 自适应滑模增益
__STATIC_INLINE float _CalcAdaptiveGain(tSMO *p, float omega_abs)
{
    float k_sl = p->cfg.k_sl_base;
    if (omega_abs > p->cfg.omega_adapt_start)
    {
        float ratio = (omega_abs - p->cfg.omega_adapt_start) /
                      (p->cfg.omega_adapt_end - p->cfg.omega_adapt_start);
        ratio = CLAMP(ratio, 0.0f, 1.0f);
        k_sl = p->cfg.k_sl_base * (p->cfg.k_sl_min_ratio +
                                   (1.0f - p->cfg.k_sl_min_ratio) * (1.0f - ratio));
    }
    return k_sl;
}

void fSmoInit(tMotor *motor)
{
    g_smo.rs = motor->rs;
    g_smo.ld = motor->ld;
    g_smo.lq = motor->lq;
    g_smo.psi_f = motor->psi_f;
    g_smo.dt = T_CON * SMO_DTICK;

    g_smo.cfg = SMO_DEFAULT_CFG;
    _SmoPrecompute(&g_smo);
    fSmoReset();

    // 滤波器初始化
    fFirstOrderLagInit(&I_alpha_lpf, CURRENT_ALPHA, 0.0f);
    fFirstOrderLagInit(&I_beta_lpf, CURRENT_ALPHA, 0.0f);
    fFirstOrderLagInit(&emf_alpha_lpf, EMF_ALPHA, 0.0f);
    fFirstOrderLagInit(&emf_beta_lpf, EMF_ALPHA, 0.0f);
    fFirstOrderLagInit(&omega_lpf, OMEGA_ALPHA, 0.0f);
}

void fSmoReset(void)
{
    g_smo.i_alpha_hat = g_smo.i_beta_hat = 0.0f;
    g_smo.e_alpha = g_smo.e_beta = 0.0f;
    g_smo.e_alpha_filt = g_smo.e_beta_filt = 0.0f;
    g_smo.theta_elec = g_smo.theta_prev = g_smo.omega_elec = 0.0f;
    g_smo.k_sl_curr = g_smo.cfg.k_sl_base;
}

void fSmoSetConfig(tSMO_Config *cfg)
{
    if (cfg == NULL)
        return;
    g_smo.cfg = *cfg;
    _SmoPrecompute(&g_smo);
}

void fSmoMainLoop(float v_alpha, float v_beta,
                  float i_alpha, float i_beta)
{
    // 空闲时刻对电流进行滤波

    if (g_smo.ts_tick++ < SMO_DTICK)
        return;
    g_smo.ts_tick = 0;

    // === 步骤 1：电流观测器 ===
#if SMO_USE_CURRENT_OBSERVER
    float i_err_alpha = i_alpha - g_smo.i_alpha_hat;
    float i_err_beta = i_beta - g_smo.i_beta_hat;
    i_err_alpha = CLAMP(i_err_alpha, -2.0f, 2.0f);
    i_err_beta = CLAMP(i_err_beta, -2.0f, 2.0f);

    float di_alpha = (v_alpha - g_smo.rs * g_smo.i_alpha_hat - g_smo.e_alpha) * g_smo.inv_l_eff;
    float di_beta = (v_beta - g_smo.rs * g_smo.i_beta_hat - g_smo.e_beta) * g_smo.inv_l_eff;

    float k_slm = g_smo.k_sl_curr * g_smo.inv_l_eff;
    di_alpha += k_slm * tanhf(i_err_alpha / g_smo.cfg.delta);
    di_beta += k_slm * tanhf(i_err_beta / g_smo.cfg.delta);

    g_smo.i_alpha_hat += di_alpha * g_smo.dt;
    g_smo.i_beta_hat += di_beta * g_smo.dt;
    g_smo.i_alpha_hat = CLAMP(g_smo.i_alpha_hat, -MAX_CURRENT, MAX_CURRENT);
    g_smo.i_beta_hat = CLAMP(g_smo.i_beta_hat, -MAX_CURRENT, MAX_CURRENT);

    g_smo.e_alpha = k_slm * tanhf(i_err_alpha / g_smo.cfg.delta);
    g_smo.e_beta = k_slm * tanhf(i_err_beta / g_smo.cfg.delta);
#else
    // 测试模式：直接用反馈电流估算反电动势
    g_smo.e_alpha = v_alpha - g_smo.rs * i_alpha;
    g_smo.e_beta = v_beta - g_smo.rs * i_beta;
    g_smo.e_alpha = CLAMP(g_smo.e_alpha, -smo.cfg.emf_max, g_smo.cfg.emf_max);
    g_smo.e_beta = CLAMP(g_smo.e_beta, -smo.cfg.emf_max, g_smo.cfg.emf_max);
    g_smo.i_alpha_hat = i_alpha;
    g_smo.i_beta_hat = i_beta;
#endif

    // === 步骤 2：自适应增益 ===
    float omega_abs = FABSF(g_smo.omega_elec);
    g_smo.k_sl_curr = _CalcAdaptiveGain(&g_smo, omega_abs);

    // === 步骤 3：反电动势滤波 ===

    g_smo.e_alpha_filt = fFirstOrderLagFilter(&emf_alpha_lpf, g_smo.e_alpha);
    g_smo.e_beta_filt = fFirstOrderLagFilter(&emf_beta_lpf, g_smo.e_beta);

    // === 步骤 4：角度计算 ===
    float emf_mag_sq = g_smo.e_alpha_filt * g_smo.e_alpha_filt +
                       g_smo.e_beta_filt * g_smo.e_beta_filt;

    if (emf_mag_sq > 0.01f)
    {
        float theta_new = atan2f(g_smo.e_beta_filt, g_smo.e_alpha_filt) * 57.29578f;
        float diff = fNormalizeAngle_180(theta_new - g_smo.theta_elec);
        g_smo.theta_elec += 0.5f * diff; // 50% 融合平滑
    }
    g_smo.theta_elec = fNormalizeAngle_0_360(g_smo.theta_elec);

    // === 步骤 5：速度计算 ===
    float angle_diff = fNormalizeAngle_180(g_smo.theta_elec - g_smo.theta_prev);
    float speed_raw = angle_diff / g_smo.dt * 0.0174533f; // deg/s -> rad/s

    g_smo.omega_elec = fFirstOrderLagFilter(&omega_lpf, speed_raw);
    g_smo.omega_elec = CLAMP(g_smo.omega_elec, -g_smo.cfg.max_omega_elec, g_smo.cfg.max_omega_elec);

    g_smo.theta_prev = g_smo.theta_elec;
}