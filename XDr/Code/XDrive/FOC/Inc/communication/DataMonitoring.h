#ifndef __DATA_MONITORING_H
#define __DATA_MONITORING_H

#include "main.h"
#define PARAMETER_LOAD_block 0
#define PARAMETER_LOAD_sector 0
#define PARAMETER_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000

#define MODE_LOAD_block 0
#define MODE_LOAD_sector 1
#define MODE_LOAD_ADDr PARAMETER_LOAD_block * 0x00010000 + PARAMETER_LOAD_sector * 0x00001000
// 反馈参数--流式数据
typedef enum
{
    STATUS, // 四个 系统状态 FOC状态 错误 警告
    TEMPERATURE,
    VBUS,
    VOLTAGE_U,
    VOLTAGE_V,
    VOLTAGE_W,
    VOLTAGE_q,
    VOLTAGE_d,

    CURRENT_U,
    CURRENT_V,
    CURRENT_W,
    CURRENT_q,
    CURRENT_d,
    CURRENT_q_ref,
    CURRENT_d_ref,

    SPEED,
    SPEED_con,
    SPEED_ref,

    THETA_elec,
    THETA_mech,
    THETA_mech_con,
    THETA_mech_ref,
} Data_stream_e;

// 读取数据流
void stream_data_get(Data_stream_e stream, float *data);

// 初始化
bool parameter_mode_init();
void STATUS_get(u8 *foc_status, u8 *fault);
#endif