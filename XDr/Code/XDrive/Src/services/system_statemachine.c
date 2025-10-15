#include "system_statemachine.h"
#include "stream_transmission.h"
#include "adcDr.h"
#include "usartDr.h"
#include "usbDr.h"
#include "canDr.h"
#include "adaptive_control.h"
#include "foc_core.h"
#include "status_feedback.h"
#include "log.h"
#include "protection_manager.h"
SYSTEM_STATE_e system_status = SYSTEM_INIT;

bool system_init_event(void)
{
    // 驱动层初始化
    ADC_DR_Init(); // 启动ADC数据刷新和foc定时器

    CANDr_Init();
    usartDrInit();
    usb_cdc_init();

    //  控制层初始化
    adaptive_control_init();
    // 服务层初始化
    if (!parameter_init())
        return false;
    log_init();
    FOC_Start_run(); // 启动FOC数据刷新

    return true;
}
void System_Run_event(void)
{
    // usb解析
    // can解析
    // uart解析
    protection_manager_run();
    status_feedback();
}
void System_Error_event(void)
{
    FOC_Stop_run(); // 停止FOC数据刷新
    System_Fault_feedback();
}
void SystemState_change(SYSTEM_STATE_e new_state)
{
    system_status = new_state;
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
        System_Run_event();
        break;
    case SYSTEM_ERROR:
        System_Error_event();
        break;
    }
}
