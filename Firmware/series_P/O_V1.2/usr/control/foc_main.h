#ifndef __FOC_STATEMACHINE_H
#define __FOC_STATEMACHINE_H

#include "foc_core.h"
#include "tune.h"
#include "hfi.h"
#include "protocol.h"

typedef struct
{
    bool foc_enable;
    bool foc_init;
    eFocState state;
    uint32_t ol_start_tick;  // 开环启动计时
    float ol_angle;           // 开环角度 [deg]
    tFOC_Core *core;
    tTuneContext *tun;
} FOC_t;
extern FOC_t g_foc;

void fFocInit();
void fFocStateMachineMainLoop();
void fFocStateUpdate(eFocState state);

#endif