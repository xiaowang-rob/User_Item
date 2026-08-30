// usr_config.h — 应用层配置
// 硬件参数请修改 board_config.h，此文件只含版本信息和计算参数
#ifndef __USR_CONFIG_H
#define __USR_CONFIG_H

// 从板级配置继承硬件参数
#include "board_config.h"

// ---------- 版本信息 ----------
#define FIRM_NAME      "XDr-P"
#define FIRM_AUTHOR    "wxd"
#define FIRM_V_DATE    "O_V1.2_260626"
#define FIRM_VERSION   FIRM_NAME " " FIRM_V_DATE

// ---------- 启动配置 ----------
#define VECT_TABLE_OFFSET  0x8000U

// ---------- 计算参数 ----------
#define F_CURRENT           (F_PWM / FREQ_HIGH_LOOP)
#define F_SPEED             (F_CURRENT / FREQ_MEDIUM_LOOP)
#define F_POSITION          (F_SPEED / FREQ_LOW_LOOP)

// ---------- 速度 LPF 滤波器系数 (2nd Order Butterworth, fc=120Hz, fs=20000Hz) ----------
// 由 Python 脚本计算，如需修改请运行 tools/filter_coeffs.py
#define LPF_W_B0    0.0003460413
#define LPF_W_B1    0.0006920827
#define LPF_W_B2    0.0003460413
#define LPF_W_A1    1.9466975408
#define LPF_W_A2    -0.9480817061

// ---------- 滤波器截止频率 ----------
#define CUR_LPF_HZ    800.0f   // 电流采样 LPF 截止
#define SPEED_LPF_HZ  50.0f    // 速度 LPF 截止

#endif // __USR_CONFIG_H
