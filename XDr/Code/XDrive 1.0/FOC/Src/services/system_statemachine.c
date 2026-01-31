#include "system_statemachine.h"
#include "foc_statemachine.h"
#include "status_feedback.h"
#include "log.h"
#include "protection_manager.h"
#include "drive_state.h"
#include "port_mapping.h"
#include "adcDr.h"
#include "flashDr.h"

SYSTEM_STATE_e system_status = SYSTEM_INIT;

bool system_init_event(void)
{
    /*
    这里的顺序不能乱，因为里面有一些初始化函数，如果顺序不对，会导致一些变量没有初始化，导致程序出错
    参数必须尽早出现 flash在其之前，初始化的时候最好不要有其他中断干涉，所有adc得往后放，但是foc初始化必须得有adc值，故最后
    正确的顺序应该是：
    flash-参数-通讯-保护-日志-adc -foc初始化
    */
    FLASH_Init();
    if (!Param_init())
        return false;
    communication_init();
    protection_manager_init();
    log_init();
    ADC_DR_Init();
    fFOC_Init();

    return true;
}

void SystemState_change(SYSTEM_STATE_e new_state)
{
    system_status = new_state;
}
SYSTEM_STATE_e SystemState_get(void)
{
    return system_status;
}
void SystemStateMachine_run(void)
{
    switch (system_status)
    {
    case SYSTEM_INIT:
        if (system_init_event())
            SystemState_change(SYSTEM_RUNNING);
        else
            SystemState_change(SYSTEM_ERROR);
        break;
    case SYSTEM_RUNNING:
        // 通讯层运行
        communication_run();
        // 控制层由定时器驱动
        // 服务层运行
        protection_manager_run();
        status_feedback();
        break;
    case SYSTEM_ERROR:
        FOC_CHANGE_STATE(FOC_FAULT);
        System_Fault_feedback();
        break;
    }
}
