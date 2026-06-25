#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "bsp.h"

void bsp_foc_it_callback(void); // FOC定时器中断回调函数声明

void bsp_power_12v_control(bool on);

// ============================================
// PWM - 电机驱动
// ============================================
void bsp_pwm_init(u32 freq_hz);
void bsp_pwm_set_duty(u8 ch, float duty);
void bsp_pwm_start(void);
void bsp_pwm_stop(void);

void bsp_pwm_set_compare(u16 ticA, u16 ticB, u16 ticC);
void bsp_pwm_enable(void);
void bsp_pwm_disable(void);

#endif // __BSP_PWM_H