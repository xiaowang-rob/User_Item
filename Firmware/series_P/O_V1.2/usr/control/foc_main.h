#ifndef __FOC_STATEMACHINE_H
#define __FOC_STATEMACHINE_H

#include "foc_core.h"
#include "loop_control.h"
#include "smo.h"
#include "tune.h"
#include "svpwm.h"
#include "hfi.h"
#include "protocol_defs.h"

typedef struct
{
    bool foc_enable;
    bool foc_init;
    eFocState state;
    tFOC_Core *core;
    tLoopControl *loop_con;
    tHFI_Handle *hfi;
    tSMO *smo;
    tTuneContext *tun;
    tSvpwm *svpwm;
} FOC_t;
extern FOC_t g_foc;

void fFocInit();
void fFocStateMachineMainLoop();
void fFocStateUpdate(eFocState state);

#endif