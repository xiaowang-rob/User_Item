#ifndef __DATA_MONITORING_H
#define __DATA_MONITORING_H

#include "main.h"
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
    POSITION,
    POSITION_ref,
} Data_stream_e;

// 读取数据流
void fStreamDataGet(Data_stream_e stream, float *data);

#endif