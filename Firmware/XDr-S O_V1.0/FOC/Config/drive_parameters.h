/* ================= drive_parameters.h ================= */
/* 此文件由 build_device_param.py 自动生成，请勿手动修改！ */
/* 生成时间: 2026-04-24 14:51:07 */
#ifndef __DRIVE_PARAMETERS_H
#define __DRIVE_PARAMETERS_H

#include "main.h"

/* ---------- 版本与产品信息 ---------- */
#define PROD_NAME           "XDr"
#define PROD_SERIES         "P"
#define FUN_V               "O"
#define FIRM_V              "V1.2"
#define BUILD_DATE_STR      "260424"
#define BUILD_DATE_NUM      260424U
#define BUILD_DATE_RAW      __DATE__
#define AUTHOR              "wxd"

/* ---------- 基本运行参数 ---------- */
#define F_PWM                     20000        // 20kHz
#define T_PWM                     5e-05f       // 50us
#define TIC_PWM                   2099         
#define T_CON                     T_PWM        
#define T_SAMPLE_us               7            // 采样 4-7us
#define T_DEATH_us                0.5f         // 死区时间
#define T_NOISE_us                0.5f         // 开关噪声时间
#define RATE_CURRENT_SAMPLE       100.0f       // 电流采样比率
#define RATE_VOLTAGE_SAMPLE       16           // 电压采样比率
#define MAX_CURRENT               100          // MOS管最大电流 100A
#define MAX_VOLTAGE               34           // 最大电压 34V
#define MIN_VOLTAGE               20           // 最小电压 20V
#define MAX_TEMPERATURE           80           // 最大工作温度
#define T_DATA_STREAM             1            // 监测数据截取周期 ms
#define T_STATE_STREAM            500          // 状态数据发送周期 ms
#define TEMP_VBUS_TS_MS           300          // 温度、电压采样周期 ms

/* ---------- 预拼接字符串 ---------- */
/* 值: "XDr-P,O_V1.2_260424,wxd,100,20-34,80" */
#define DRIVE_MESSAGE       "XDr-P,O_V1.2_260424,wxd,100,20-34,80"

/* 值: "XDr-P O_V1.2_260424" */
#define FIRM_VERSION        "XDr-P O_V1.2_260424"

#endif /* __DRIVE_PARAMETERS_H */
