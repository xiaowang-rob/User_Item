#ifndef __SYSTEM_PARAMETERS_H
#define __SYSTEM_PARAMETERS_H

#define fpwm 20000
#define Tpwm 1.0f / fpwm
#define ticpwm 4200 / 2 - 1
#define Tcon Tpwm

#define rate_CurrentSample 42.6749f // 电流采样 电流与电压比值

#endif