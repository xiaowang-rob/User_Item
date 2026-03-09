#ifndef __FOC_STATEMACHINE_H
#define __FOC_STATEMACHINE_H

#include "main.h"
#include "foc_core.h"
#include "loop_control.h"
#include "smo.h"
#include "tune.h"
#include "svpwm.h"
typedef enum
{
    FOC_IDLE,      // 用于参数调节、模式调节 状态
    FOC_AUTO_TUNE, // 自动校准电机参数 状态
    FOC_RESET,     // 复位电机动作 过程
    FOC_ENABLE,    // 使能电机动作 过程
    FOC_DISABLE,   // 禁用电机动作 过程
    FOC_RUNNING,   // FOC参数计算 PWM输出 状态
    FOC_SHUTDOWN,  // 紧急停止电机动作 过程
    FOC_FAULT,     // 故障状态
} eFOC_Status;

typedef struct
{
    bool foc_enable;
    eFOC_Status state;
    tFOC_Core *core;
    tLoopControl *loop_con;
    tSMO *smo;
    tTuneContext *tun;
    tSvpwm *svpwm;
} FOC_t;
extern FOC_t g_foc;

void fFOC_Init();
void fFOC_StateMachineMainLoop();
void fFOC_StateUpdate(eFOC_Status state);

#endif