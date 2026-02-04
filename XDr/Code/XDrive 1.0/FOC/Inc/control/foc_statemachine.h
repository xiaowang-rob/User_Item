#ifndef __FOC_STATEMACHINE_H
#define __FOC_STATEMACHINE_H

#include "main.h"
#include "foc_core.h"
#include "loop_control.h"
#include "smo.h"
#include "svpwm.h"
typedef enum
{
    FOC_IDLE,
    FOC_AUTO_TUNE,
    FOC_RESET,
    FOC_ENABLE,
    FOC_DISABLE,
    FOC_RUNNING,
    FOC_SHUTDOWN,
    FOC_FAULT,
    FOC_WARNING,
} FOC_STATE_e;

typedef struct
{
    bool foc_enable;
    FOC_STATE_e state;
    FOC_mode_t *mode;
    FOC_val_t *val;
    startup_mechine_t *startup_mechine;
    LOOP_CON_t *g_loop_con;
    smo_t *smo;
    param_tuning_t *tun;
    tSvpwm *svpwm;
    Motor_t *motor;
} FOC_t;
extern FOC_t g_foc;

void fFOC_Init();
void FOC_StateMachine_updata();
void FOC_CHANGE_STATE(FOC_STATE_e state);
void FOC_Start_run();
void FOC_Stop_run();

#endif