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
extern SVPWM_t svpwm_g;

void svpwm_Init(float Vbus);
void svpwm(float ualpha, float ubeta);
void svpwm_SetVbus(float Vbus);

#endif