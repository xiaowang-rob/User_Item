#include "foc_statemachine.h"
#include "auto_calibration.h"
#include "tim.h"
#include "math_fast.h"
#include "parameter_manager.h"

FOC_t foc = {0};

#ifdef __DEBUG__
u32 _time_focit_start = 0;
u32 _time_focit_end = 0;
u32 _time_focit_run = 0;
u32 _time_foc_T = 0;

u32 _time_curadc_zero = 0;
u32 _time_curadc_T = 0;

#endif

// 由于tim8互补波输出的原因 计时器模式被强制改为中心对齐3 导致update回调被重复调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
        if (TIM8->CR1 & TIM_CR1_DIR)
        {
            // ========== 上溢中断 ==========
            return;
        }
        else
        {
            // ========== 下溢中断 ==========
#ifdef __DEBUG__
            _time_focit_start = HAL_GetTick_us();

#endif
            FOC_StateMachine_updata();
#ifdef __DEBUG__
            _time_foc_T = HAL_GetTick_us() - _time_focit_end;
            _time_focit_end = HAL_GetTick_us();
            _time_focit_run = _time_focit_end - _time_focit_start;
#endif
            return;
        }
    }
}

void FOC_INIT()
{
    foc.state = FOC_IDLE;
    foc.mode = FOC_GET_MODE_adr();
    foc.val = FOC_GET_VAL_adr();
    foc.startup_mechine = FOC_GET_STARTUP_adr();
    foc.g_loop_con = get_loop_con_adr();
    foc.smo = get_smo_adr();
    foc.tun = get_tuning_adr();
    foc.svpwm = get_svpwm_adr();
    foc.motor = get_motor_adr();
    foc_core_init(g_Param);
    PWM_POWER_ON();
}

void FOC_StateMachine_updata()
{
    FOC_PREPARE();
    switch (foc.state)
    {
    case FOC_IDLE:
        break;
    case FOC_AUTO_TUNE:
        foc_core_run();
        if (auto_calibration_update())
            FOC_CHANGE_STATE(FOC_IDLE);
        break;
    case FOC_RESET:
        foc_core_reset();
        FOC_CHANGE_STATE(FOC_IDLE);
        break;
    case FOC_ENABLE:
        foc.ENABLE = true;
        ENABLE_PWM();
        FOC_CHANGE_STATE(FOC_RUNNING);
        break;
    case FOC_DISABLE:
        foc.ENABLE = false;
        DISABLE_PWM();
        FOC_CHANGE_STATE(FOC_RESET);
        break;
    case FOC_RUNNING:
        FOC_RUN();
        break;
    case FOC_SHUTDOWN:
        if (SHUTDOWM())
            FOC_CHANGE_STATE(FOC_FAULT);
        break;
    case FOC_FAULT:
        if (foc.ENABLE)
        {
            foc.ENABLE = false;
            DISABLE_PWM();
            PWM_POWER_OFF();
        }
        break;
    default:
        break;
    }
}
void FOC_CHANGE_STATE(FOC_STATE_e state)
{
    foc.state = state;
}
FOC_STATE_e FOC_Get_state()
{
    return foc.state;
}
