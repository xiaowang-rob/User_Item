#ifndef __SVPWM_H
#define __SVPWM_H

#include "main.h"

typedef struct
{
    float k;
    bool power_flag;
    uint8_t sector;
    uint16_t ticu;
    uint16_t ticv;
    uint16_t ticw;
} tSvpwm;

extern tSvpwm svpwm;

// PWM 硬件控制
void ENABLE_PWM(void);
void DISABLE_PWM(void);
void PWM_POWER_ON(void);
void PWM_POWER_OFF(void);

// SVPWM 核心接口
void fSvpwmInit(float Vbus);
void fSvpwmRun(float ualpha, float ubeta);
void fSamplePointCalibration(void);
void fSvpwmSetVbus(float Vbus);
uint8_t fSvpwmGetSector(void);

// 调试接口（理论电压）
float fGetVoltage_u(void);
float fGetVoltage_v(void);
float fGetVoltage_w(void);

#endif /* __SVPWM_H */