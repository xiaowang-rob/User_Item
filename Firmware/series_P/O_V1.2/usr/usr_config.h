/* ===== 此文件由 build.py 自动生成，请勿手动修改，相关配置在 usr_config.json 中 ===== */
/* 生成时间: 2026-05-16 16:07:08 */
#ifndef __USR_CONFIG_H
#define __USR_CONFIG_H


/* ---------- 版本信息 ---------- */
#define FIRM_NAME      "XDr-P"
#define FIRM_AUTHOR    "wxd"
#define FIRM_V_DATE    "O_V1.2_260516"
#define FIRM_VERSION   FIRM_NAME " " FIRM_V_DATE

/* ---------- 启动配置 ---------- */
#define VECT_TABLE_OFFSET  0x8000U

/* ---------- 控制参数 ---------- */
#define F_PWM               20000.0
#define T_PWM               0.00005
#define TIC_PWM             2099
#define T_CON               0.00005

#define FREQ_CURRENT        1
#define FREQ_SPEED          10
#define FREQ_POSITION       10

#define F_CURRENT           20000.0
#define F_SPEED             2000.0
#define F_POSITION          200.0

#define T_DATA_STREAM       1
#define T_STATE_STREAM      500
#define TEMP_VBUS_TS_MS     300

/* ---------- 硬件限制参数 (来自 BSP) ---------- */
#define MAX_CURRENT         100
#define MAX_VOLTAGE         34
#define MIN_VOLTAGE         20
#define MAX_TEMPERATURE     80
#define T_SAMPLE_us         7.0
#define T_DEATH_us          0.5
#define T_NOISE_us          0.5

/* ---------- 速度 LPF 滤波器系数 (Butterworth) ---------- */
/* 速度 LPF — 2nd Order Butterworth LPF, fc=80.0Hz, fs=20000Hz */
#define LPF_W_B0    0.0001551484
#define LPF_W_B1    0.0003102968
#define LPF_W_B2    0.0001551484
#define LPF_W_A1    1.9644605802
#define LPF_W_A2    -0.9650811739

#endif /* __USR_CONFIG_H */
