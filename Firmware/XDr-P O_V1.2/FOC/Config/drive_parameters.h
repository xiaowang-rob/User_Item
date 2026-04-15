#ifndef __DRIVE_PARAMETERS_H
#define __DRIVE_PARAMETERS_H

#include "main.h"

//硬件版本信息
#define Description "XDr-P"
#define FUN_V "O" //功能版本
#define FIRM_V "V1.2" //硬件版本
#define BULID_T "260414"
#define VERSION FUN_V "_" FIRM_V "_" BULID_T
#define AUTHOR "wxd"
#define MAX_CURRENT_string "100"
#define Voltage_string "20-30"
#define MAX_Temperature_string "90"
#define DRIVE_DESC_str \
    Description "," VERSION "," AUTHOR "," MAX_CURRENT_string "," Voltage_string "," MAX_Temperature_string


//基本运行参数
#define fpwm 20000    // 20kHz
#define Tpwm 0.00005f // 50us
#define ticpwm 2099
#define Tcon Tpwm

#define Tsample_us 7   // 采样 4-7us
#define Tdeath_us 0.5f // 死区时间 300ns 栅极驱动芯片200ns pwm互补波100ns 取0.5us
#define Tnoise_us 0.5f // 开关噪声时间 300ns 取0.5us

#define rate_CurrentSample 100.f // 电流采样 电流与adc电压比值 1/20(差分放大倍数)/0.0005（采样电阻阻值）
#define rate_VoltageSample 16    // 电压采样 分压电阻 缩小倍数 （10+150）/10 16

#define MAX_Current 100    // MOS管最大电流 100A
#define MAX_Voltage 34     // 最大电压 34V
#define MIN_Voltage 20     // 最小电压 20V 电压过低影响控制
#define MAX_Temperature 80 // 最大工作温度

#define DATA_stream_T 1     // 监测数据截取周期 ms
#define STATE_stream_T 500  // 状态数据发送周期 ms
#define TEMP_VBUS_TS_MS 100 // 温度、电压采样周期 ms

#endif