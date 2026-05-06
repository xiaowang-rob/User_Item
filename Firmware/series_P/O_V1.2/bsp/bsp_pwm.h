#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "bsp.h"

void BSP_FOC_ITCallback(void); // FOC定时器中断回调函数声明

void BSP_POWER_12V_Control(bool on);

/* ============================================
 * PWM - 电机驱动
 * ============================================ */
void BSP_PwmInit(u32 freq_hz);
void BSP_PwmSetDuty(u8 ch, float duty);
void BSP_PwmStart(void);
void BSP_PwmStop(void);

void BSP_PWM_SetCompare(u16 ticA, u16 ticB, u16 ticC);
void BSP_PWM_Enable(void);
void BSP_PWM_Disable(void);

#endif /* __BSP_PWM_H */