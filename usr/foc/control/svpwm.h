#ifndef __SVPWM_H
#define __SVPWM_H

#include "bsp_interface.h"

typedef struct
{
    float k;
    u8 sector;
    u16 ticu;
    u16 ticv;
    u16 ticw;
} tSvpwm;

extern tSvpwm svpwm;

// SVPWM 核心接口
void fSvpwmInit(float Vbus);
void fSvpwmRun(float ualpha, float ubeta);
void fSamplePointCalibration(void);
void fSvpwmSetVbus(float Vbus);
u8 fSvpwmGetSector(void);

// 调试接口（理论电压）
float fGetVoltage_u(void);
float fGetVoltage_v(void);
float fGetVoltage_w(void);

#endif /* __SVPWM_H */