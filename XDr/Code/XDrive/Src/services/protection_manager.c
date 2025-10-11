#include "protection_manager.h"
#include "foc_core.h"
#include "system_parameters.h"
#include "foc_statemachine.h"
#include "adaptive_control.h"
#include "encoder.h"
#include "log.h"
#include "adcDr.h"

protection_manager_t protection_manager = {0};
u32 _time = 0;
u32 _time_last = 0;
bool Tolerance_check(float *value, float max_value, float min_value, float tolerance)
{
    if (*value > max_value * tolerance || *value < min_value * tolerance)
    {
        _time += (HAL_GetTick() - _time_last);
        if (_time > protection_manager.tolerance_time)
            return true;
    }
    else if (_time != 0)
    {
        if (_time > 1)
            _time -= (HAL_GetTick() - _time_last);
        else
            _time = 0;
    }
    return false;
}
void protection_manager_init(float maxcurrent, float max_speed, float min_position, float max_position, float tolerance_time, float tolerance_voltage, float tolerance_current, float tolerance_speed, float tolerance_position)
{
}
void protection_manager_run()
{
    if (protection_manager.clear_fault)
    {
        protection_manager.clear_fault = false;
        protection_manager.serious_fault = false;
    }
    if (protection_manager.serious_fault)
        return;
    if (g_monitor.Iu > MAX_Current || g_monitor.Iv > MAX_Current || g_monitor.Iw > MAX_Current || Tolerance_check(&g_monitor.iq_fb, protection_manager.maxcurrent, 0, protection_manager.tolerance_current))
    {
        protection_manager.fault = OVER_CURRENT;
        protection_manager.serious_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    if (g_adaptive_con.tempareture > MAX_Temperature)
    {
        protection_manager.fault = OVER_TEMPERATURE;
        protection_manager.serious_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    float Udc = ADC_GET_Voltage();
    if (Tolerance_check(&Udc, MAX_Voltage, MIN_Voltage, protection_manager.tolerance_voltage) && Udc > MAX_Voltage)
    {
        protection_manager.fault = OVER_VOLTAGE;
        protection_manager.warning_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (protection_manager.fault == OVER_VOLTAGE)
    {
        protection_manager.warning_fault = false;
        protection_manager.fault = NO_FAULT;
        protection_manager.log_done = false;
    }
    if (Udc < MIN_Voltage && Tolerance_check(&Udc, MAX_Voltage, MIN_Voltage, protection_manager.tolerance_voltage))
    {
        protection_manager.fault = UNDER_VOLTAGE;
        protection_manager.warning_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (protection_manager.fault == UNDER_VOLTAGE)
    {
        protection_manager.warning_fault = false;
        protection_manager.fault = NO_FAULT;
        protection_manager.log_done = false;
    }
    // todo:获取can状态
    if (GET_ENCODER_NO_MAG_FLAG())
    {
        protection_manager.fault = ENCODER_MAG_WEAK;
        protection_manager.warning_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (protection_manager.fault == ENCODER_MAG_WEAK)
    {
        protection_manager.warning_fault = false;
        protection_manager.fault = NO_FAULT;
        protection_manager.log_done = false;
    }
    if (GET_ENCODER_COMMUNICATION_ERROR())
    {
        protection_manager.fault = ENCODER_COMMUNICATION_FAULT;
        protection_manager.warning_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (protection_manager.fault == ENCODER_COMMUNICATION_FAULT)
    {
        protection_manager.warning_fault = false;
        protection_manager.fault = NO_FAULT;
        protection_manager.log_done = false;
    }
    if ((protection_manager.warning_fault || protection_manager.serious_fault) && !protection_manager.log_done)
    {
        // todo:如果非上位机状态 再写日志 上位机状态直接实时显示
        log_write();
        protection_manager.log_done = true;
    }
}
fault_t GET_Protect_fault()
{
    return protection_manager.fault;
}