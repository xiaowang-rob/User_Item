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
extern SVPWM_t g_svpwm;

void ENABLE_PWM();
void DISABLE_PWM();
void svpwm_Init(SVPWM_t svpwm, float Vbus);
void svpwm_run(float ualpha, float ubeta, SVPWM_t svpwm);
void svpwm_SetVbus(SVPWM_t svpwm, float Vbus);

#endif