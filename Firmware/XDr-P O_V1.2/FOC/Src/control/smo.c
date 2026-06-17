#include "smo.h"
#include "math_fast.h"
#include "drive_parameters.h"
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

tSMO smo;

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
__STATIC_INLINE void fSMO_Precompute(tSMO *p)
{
    float L_avg = (p->Ld + p->Lq) * 0.5f;
    p->inv_L_eff = 1.0f / (L_avg + p->Rs * p->dt);
}

// 自适应滑模增益
__STATIC_INLINE float fSMO_CalcAdaptiveGain(tSMO *p, float omega_abs)
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

void fSMO_Init(tMotor *motor)
{
    smo.Rs = motor->Rs;
    smo.Ld = motor->Ld;
    smo.Lq = motor->Lq;
    smo.Psi_f = motor->Psi_f;
    smo.dt = T_CON * SMO_DTICK;

    smo.cfg = SMO_DEFAULT_CFG;
    fSMO_Precompute(&smo);
    fSMO_Reset();

    // 滤波器初始化
    fFirstOrderLagInit(&I_alpha_lpf, CURRENT_ALPHA, 0.0f);
    fFirstOrderLagInit(&I_beta_lpf, CURRENT_ALPHA, 0.0f);
    fFirstOrderLagInit(&emf_alpha_lpf, EMF_ALPHA, 0.0f);
    fFirstOrderLagInit(&emf_beta_lpf, EMF_ALPHA, 0.0f);
    fFirstOrderLagInit(&omega_lpf, OMEGA_ALPHA, 0.0f);
}

void fSMO_Reset(void)
{
    smo.i_alpha_hat = smo.i_beta_hat = 0.0f;
    smo.e_alpha = smo.e_beta = 0.0f;
    smo.e_alpha_filt = smo.e_beta_filt = 0.0f;
    smo.theta_elec = smo.theta_prev = smo.omega_elec = 0.0f;
    smo.k_sl_curr = smo.cfg.k_sl_base;
}

void fSMO_SetConfig(tSMO_Config *cfg)
{
    if (cfg == NULL)
        return;
    smo.cfg = *cfg;
    fSMO_Precompute(&smo);
}

void fSMO_MainLoop(float v_alpha, float v_beta,
                   float i_alpha, float i_beta)
{
    // 空闲时刻对电流进行滤波

    if (smo.Ts_tick++ < SMO_DTICK)
        return;
    smo.Ts_tick = 0;

    // === 步骤 1：电流观测器 ===
#if SMO_USE_CURRENT_OBSERVER
    float i_err_alpha = i_alpha - smo.i_alpha_hat;
    float i_err_beta = i_beta - smo.i_beta_hat;
    i_err_alpha = CLAMP(i_err_alpha, -2.0f, 2.0f);
    i_err_beta = CLAMP(i_err_beta, -2.0f, 2.0f);

    float di_alpha = (v_alpha - smo.Rs * smo.i_alpha_hat - smo.e_alpha) * smo.inv_L_eff;
    float di_beta = (v_beta - smo.Rs * smo.i_beta_hat - smo.e_beta) * smo.inv_L_eff;

    float k_slm = smo.k_sl_curr * smo.inv_L_eff;
    di_alpha += k_slm * tanhf(i_err_alpha / smo.cfg.delta);
    di_beta += k_slm * tanhf(i_err_beta / smo.cfg.delta);

    smo.i_alpha_hat += di_alpha * smo.dt;
    smo.i_beta_hat += di_beta * smo.dt;
    smo.i_alpha_hat = CLAMP(smo.i_alpha_hat, -MAX_CURRENT, MAX_CURRENT);
    smo.i_beta_hat = CLAMP(smo.i_beta_hat, -MAX_CURRENT, MAX_CURRENT);

    smo.e_alpha = k_slm * tanhf(i_err_alpha / smo.cfg.delta);
    smo.e_beta = k_slm * tanhf(i_err_beta / smo.cfg.delta);
#else
    // 测试模式：直接用反馈电流估算反电动势
    smo.e_alpha = v_alpha - smo.Rs * i_alpha;
    smo.e_beta = v_beta - smo.Rs * i_beta;
    smo.e_alpha = CLAMP(smo.e_alpha, -smo.cfg.emf_max, smo.cfg.emf_max);
    smo.e_beta = CLAMP(smo.e_beta, -smo.cfg.emf_max, smo.cfg.emf_max);
    smo.i_alpha_hat = i_alpha;
    smo.i_beta_hat = i_beta;
#endif

    // === 步骤 2：自适应增益 ===
    float omega_abs = FABSF(smo.omega_elec);
    smo.k_sl_curr = fSMO_CalcAdaptiveGain(&smo, omega_abs);

    // === 步骤 3：反电动势滤波 ===

    smo.e_alpha_filt = fFirstOrderLagFilter(&emf_alpha_lpf, smo.e_alpha);
    smo.e_beta_filt = fFirstOrderLagFilter(&emf_beta_lpf, smo.e_beta);

    // === 步骤 4：角度计算 ===
    float emf_mag_sq = smo.e_alpha_filt * smo.e_alpha_filt +
                       smo.e_beta_filt * smo.e_beta_filt;

    if (emf_mag_sq > 0.01f)
    {
        float theta_new = atan2f(smo.e_beta_filt, smo.e_alpha_filt) * 57.29578f;
        float diff = fNormalizeAngle_180(theta_new - smo.theta_elec);
        smo.theta_elec += 0.5f * diff; // 50% 融合平滑
    }
    smo.theta_elec = fNormalizeAngle_0_360(smo.theta_elec);

    // === 步骤 5：速度计算 ===
    float angle_diff = fNormalizeAngle_180(smo.theta_elec - smo.theta_prev);
    float speed_raw = angle_diff / smo.dt * 0.0174533f; // deg/s -> rad/s

    smo.omega_elec = fFirstOrderLagFilter(&omega_lpf, speed_raw);
    smo.omega_elec = CLAMP(smo.omega_elec, -smo.cfg.max_omega_elec, smo.cfg.max_omega_elec);

    smo.theta_prev = smo.theta_elec;
}