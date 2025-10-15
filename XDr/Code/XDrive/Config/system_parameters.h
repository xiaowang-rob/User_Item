#ifndef __SYSTEM_PARAMETERS_H
#define __SYSTEM_PARAMETERS_H

/*
人为输入 转速用rpm   角度用°
时间用ms
*/
#define Description "XDr-standard"
#define VERSION "1.0.0"
#define AUTHOR "xiaowang"
#define Frequency_string "20kHz"
#define MAX_CURRENT_string "50A"
#define MAX_Voltage_string "34V"
#define MIN_Voltage_string "20V"
#define MAX_Temperature_string "80"

#define fpwm 20000
#define Tpwm 1.0f / fpwm // 50us
#define ticpwm 4200 / 2 - 1
#define Tcon Tpwm
#define Tadc 8.f / 84000000.f
#define Ts 14 * Tadc                       // 14个ADC周期 1.3us
#define tics Ts / Tpwm *ticpwm             // 14个ADC周期 1.3us 4200/2-1 109个计数值
#define DT 0.3f / 1000000.f                // 死区时间 300ns 栅极驱动芯片200ns pwm互补波100ns
#define ticDT DT / Tpwm *ticpwm            // 300ns 100ns 25个计数值
#define TN 0.3f / 1000000.f                // 开关噪声时间 300ns
#define ticTN TN / Tpwm *ticpwm            // 300ns 25个计数值
#define alltic_tsdttn tics + ticDT + ticTN // 总计数值

#define rate_CurrentSample 42.6749f // 电流采样 电流与电压比值
#define MAX_Current 50
#define MAX_Voltage 34
#define MIN_Voltage 20
#define MAX_Temperature 80

#endif