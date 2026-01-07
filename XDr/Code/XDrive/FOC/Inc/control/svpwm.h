#ifndef __SVPWM_H
#define __SVPWM_H

#include "main.h"

typedef struct
{
    float k;
    u8 sector;
    u16 ticu;
    u16 ticv;
    u16 ticw;
} SVPWM_t;
SVPWM_t *get_svpwm_adr();
void ENABLE_PWM();
void DISABLE_PWM();
void PWM_POWER_ON();
void PWM_POWER_OFF();
void svpwm_Init(float Vbus);
void svpwm_run(float ualpha, float ubeta);
void smaple_point_change();
void svpwm_SetVbus(float Vbus);
u8 svpwm_GetSector();

#endif