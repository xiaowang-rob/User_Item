/* ===== 此文件由 build.py 自动生成，请勿手动修改 ===== */
/* 生成时间: 2026-06-16 00:04:48 */
#ifndef __USR_CONFIG_H
#define __USR_CONFIG_H

#include "bsp.h"

/* ---------- 版本信息 ---------- */
#define FIRM_NAME      "XDr-bl"
#define FIRM_AUTHOR    "wxd"
#define FIRM_V_DATE    "O_V1.2_260616"
#define FIRM_VERSION   FIRM_NAME " " FIRM_V_DATE

/* ---------- 启动配置 ---------- */
#define VECT_TABLE_OFFSET  0x0U  /* 向量表偏移 */

/* ---------- 控制参数 ---------- */
#define F_PWM               20000.0f
#define T_PWM               0.00005f
#define TIC_PWM             2099
#define T_CON               0.0f

#define FREQ_CURRENT        1
#define FREQ_SPEED          10
#define FREQ_POSTION        10

#define F_CURRENT           20000.0f
#define F_SPEED             2000.0f
#define F_POSITION          200.0f

#define T_DATA_STREAM       1
#define T_STATE_STREAM      500
#define TEMP_VBUS_TS_MS     300

/* ---------- 硬件限制参数 (来自 BSP) ---------- */
#define MAX_CURRENT         100       /* MOS管最大电流 (A) */
#define MAX_VOLTAGE         34       /* 最大电压 (V) */
#define MIN_VOLTAGE         20       /* 最小电压 (V) */
#define MAX_TEMPERATURE     80   /* 最大工作温度 (°C) */
#define T_SAMPLE_us         7.0f  /* 采样时间 (us) */
#define T_DEATH_us          0.5f   /* 死区时间 (us) */
#define T_NOISE_us          0.5f   /* 开关噪声时间 (us) */

/* ---------- 巴特沃斯滤波器系数 (CMSIS-DSP, [b0, b1, b2, a1, a2]) ---------- */
/* 速度 LPF — 2nd Order Butterworth LPF, fc=40.0Hz, fs=20000Hz */
#define LPF_W_B0    0.0000391302f
#define LPF_W_B1    0.0000782604f
#define LPF_W_B2    0.0000391302f
#define LPF_W_A1    1.9822289298f
#define LPF_W_A2    -0.9823854506f

/* 电流采样 LPF — 2nd Order Butterworth LPF, fc=3200.0Hz, fs=20000Hz */
#define LPF_I_B0    0.1453238839f
#define LPF_I_B1    0.2906477678f
#define LPF_I_B2    0.1453238839f
#define LPF_I_A1    0.6710290908f
#define LPF_I_A2    -0.2523246263f

/* ========== 通讯协议 (固件/上位机共用) ========== */

/* ---------- CMD ID ---------- */
#define UC_CONNECT               0xf0    /* 上位机连接 */
#define UC_DISCONNECT            0xfe    /* 上位机断开 */
#define START_TUNNING            0xf1    /* 开始调参 */
#define BRAKE                    0xf2    /* 刹车 */
#define FOC_NRST                 0xf3    /* FOC复位 */
#define CMD_ENABLE               0xf4    /* 电机使能 */
#define CMD_DISABLE              0xf5    /* 电机失能 */
#define PROTECT_RESET            0xf6    /* 保护机制复位 */
#define LOG_GET                  0xf7    /* 获取日志 */
#define LOG_ERASE                0xf8    /* 日志擦除 */
#define PARAM_ERASE              0x01    /* 参数擦除 */
#define PARAM_WRITE              0x02    /* 参数写入 */
#define PARAM_READ               0x03    /* 参数读取 */
#define PARAM_SAVE               0x04    /* 参数保存 */
#define CMD_REFVALUE_SET         0x21    /* 目标值设置 */
#define CMD_MODE_SET             0x22    /* 模式设置 */
#define CMD_STREAM_GET           0x23    /* 监测值获取 */
#define CMD_STREAM_SET           0x25    /* 数据流设置 */
#define CMD_SET_ZERO_POS         0x26    /* 设置零点 */
#define CMD_SET_LIMIT_POS        0x27    /* 设置极限位置 */
#define CMD_HANDSHAKE            0x28    /* CAN握手 */
#define CMD_SYSTEM_RESET         0x30    /* 系统复位 */
#define CMD_IAP_ENTER            0x31    /* 进入IAP模式 */
#define CMD_IAP_ERASE_FLASH      0x32    /* 擦除Flash */
#define CMD_IAP_WRITE_FLASH      0x33    /* 写入Flash */
#define CMD_IAP_VERIFY_FLASH     0x34    /* 校验Flash */
#define CMD_IAP_EXIT             0x35    /* 退出IAP模式 */


/* ---------- 反馈 ID ---------- */
#define FEEDBACK_EXECUTE         0xf0
#define FEEDBACK_FAILURE         0xfe


/* ---------- USB 协议格式 ---------- */
#define USB_PACKET_HEAD          0x55
#define USB_PACKET_TAIL          0xAA


/* ---------- UART 协议格式 ---------- */
#define PACKET_HEAD              0x55
#define PACKET_TAIL              0xAA


/* ---------- CAN 协议 ---------- */
/* 默认唯一扩展帧 ID, 频率 1M */

/* ---------- 帧长度 ---------- */
#define MAX_FRAME_LENGTH         128


/* ---------- 数据流 ID ---------- */
typedef enum {
    STREAM_STATUS,    /* 系统状态 | FOC状态 | 错误 | 警告 */
    STREAM_TEMPERATURE,
    STREAM_VBUS,
    STREAM_VOLTAGE_U,
    STREAM_VOLTAGE_V,
    STREAM_VOLTAGE_W,
    STREAM_VOLTAGE_Q,
    STREAM_VOLTAGE_D,
    STREAM_CURRENT_ALPHA,
    STREAM_CURRENT_BETA,
    STREAM_CURRENT_Q,
    STREAM_CURRENT_D,
    STREAM_CURRENT_Q_REF,
    STREAM_CURRENT_D_REF,
    STREAM_SPEED,
    STREAM_SPEED_REF,
    STREAM_THETA_ELEC,
    STREAM_THETA_MECH,
    STREAM_POSITION,
    STREAM_POSITION_REF
} DataStreamId;

#endif /* __USR_CONFIG_H */
