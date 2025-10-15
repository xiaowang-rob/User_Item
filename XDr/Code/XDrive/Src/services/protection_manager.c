#include "protection_manager.h"
#include "foc_core.h"
#include "system_parameters.h"
#include "foc_statemachine.h"
#include "adaptive_control.h"
#include "encoder.h"
#include "log.h"
#include "adcDr.h"
#include "auto_calibration.h"
protection_manager_t g_protection_manager = {0};
u32 _time = 0;
u32 _time_last = 0;
bool Tolerance_check(float *value, float max_value, float min_value, float tolerance)
{
    if (*value > max_value * tolerance || *value < min_value * tolerance)
    {
        _time += (HAL_GetTick() - _time_last);
        if (_time > g_protection_manager.tolerance_time)
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
void protection_manager_init(float maxcurrent, float max_speed, float min_position, float max_position,
                             float tolerance_time, float tolerance_voltage, float tolerance_current, float tolerance_speed,
                             float tolerance_position)
{
    g_protection_manager.maxcurrent = maxcurrent;
    g_protection_manager.maxspeed = max_speed;
    g_protection_manager.minposition = min_position;
    g_protection_manager.maxposition = max_position;
    g_protection_manager.tolerance_time = tolerance_time;
    g_protection_manager.tolerance_voltage = tolerance_voltage;
    g_protection_manager.tolerance_current = tolerance_current;
    g_protection_manager.tolerance_speed = tolerance_speed;
    g_protection_manager.tolerance_position = tolerance_position;
    g_protection_manager.serious_fault = false;
    g_protection_manager.warning_fault = false;
    g_protection_manager.clear_fault = false;
    g_protection_manager.log_done = false;
}
void protection_manager_run()
{
    if (g_protection_manager.clear_fault)
    {
        g_protection_manager.clear_fault = false;
        g_protection_manager.serious_fault = false;
    }
    if (g_protection_manager.serious_fault)
        return;
    if (get_motor_fault_flag())
    {
        g_protection_manager.fault = MOTOR_ERROR;
        g_protection_manager.serious_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }

    if (g_monitor.Iu > MAX_Current || g_monitor.Iv > MAX_Current || g_monitor.Iw > MAX_Current || Tolerance_check(&g_monitor.iq_fb, g_protection_manager.maxcurrent, 0, g_protection_manager.tolerance_current))
    {
        g_protection_manager.fault = OVER_CURRENT;
        g_protection_manager.serious_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    if (g_adaptive_con.tempareture > MAX_Temperature)
    {
        g_protection_manager.fault = OVER_TEMPERATURE;
        g_protection_manager.serious_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    if (Tolerance_check(&g_adaptive_con.Udc, MAX_Voltage, MIN_Voltage, g_protection_manager.tolerance_voltage) && g_adaptive_con.Udc > MAX_Voltage)
    {
        g_protection_manager.fault = OVER_VOLTAGE;
        g_protection_manager.warning_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (g_protection_manager.fault == OVER_VOLTAGE)
    {
        g_protection_manager.warning_fault = false;
        g_protection_manager.fault = NO_FAULT;
        g_protection_manager.log_done = false;
    }
    if (g_adaptive_con.Udc < MIN_Voltage && Tolerance_check(&g_adaptive_con.Udc, MAX_Voltage, MIN_Voltage, g_protection_manager.tolerance_voltage))
    {
        g_protection_manager.fault = UNDER_VOLTAGE;
        g_protection_manager.warning_fault = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (g_protection_manager.fault == UNDER_VOLTAGE)
    {
        g_protection_manager.warning_fault = false;
        g_protection_manager.fault = NO_FAULT;
        g_protection_manager.log_done = false;
    }
    // todo:获取can状态
    if (g_foccore.run_mode == ENCODER_CONTROL)
    { // 有感模式启动编码器判断
        if (GET_ENCODER_NO_MAG_FLAG())
        {
            g_protection_manager.fault = ENCODER_MAG_WEAK;
            g_protection_manager.warning_fault = true;
            FOC_CHANGE_STATE(FOC_FAULT);
        }
        else if (g_protection_manager.fault == ENCODER_MAG_WEAK)
        {
            g_protection_manager.warning_fault = false;
            g_protection_manager.fault = NO_FAULT;
            g_protection_manager.log_done = false;
        }
        if (GET_ENCODER_COMMUNICATION_ERROR())
        {
            g_protection_manager.fault = ENCODER_COMMUNICATION_FAULT;
            g_protection_manager.warning_fault = true;
            FOC_CHANGE_STATE(FOC_FAULT);
        }
        else if (g_protection_manager.fault == ENCODER_COMMUNICATION_FAULT)
        {
            g_protection_manager.warning_fault = false;
            g_protection_manager.fault = NO_FAULT;
            g_protection_manager.log_done = false;
        }
    }
    if ((g_protection_manager.warning_fault || g_protection_manager.serious_fault) && !g_protection_manager.log_done)
    {
        // todo:如果非上位机状态 再写日志 上位机状态直接实时显示
        log_write();
        g_protection_manager.log_done = true;
    }
}

fault_e GET_Protect_fault()
{
    return g_protection_manager.fault;
}