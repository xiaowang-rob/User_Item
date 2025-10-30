#include "protection_manager.h"
#include "foc_core.h"
#include "system_parameters.h"
#include "foc_statemachine.h"
#include "adaptive_control.h"
#include "encoder.h"
#include "log.h"
#include "adcDr.h"
#include "auto_calibration.h"
#include "canDr.h"
#include "usbDr.h"
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
    g_protection_manager.fault = NO_FAULT;
    g_protection_manager.warning = NO_WARNING;
    g_protection_manager.fault_flag = false;
    g_protection_manager.warning_flag = false;
    g_protection_manager.log_done = false;
    g_protection_manager.maxcurrent = maxcurrent;
    g_protection_manager.maxspeed = max_speed;
    g_protection_manager.minposition = min_position;
    g_protection_manager.maxposition = max_position;
    g_protection_manager.tolerance_time = tolerance_time;
    g_protection_manager.tolerance_voltage = tolerance_voltage;
    g_protection_manager.tolerance_current = tolerance_current;
    g_protection_manager.tolerance_speed = tolerance_speed;
    g_protection_manager.tolerance_position = tolerance_position;
}
void protection_manager_clear_fault()
{
    g_protection_manager.fault_flag = false;
    g_protection_manager.warning_flag = false;
    FOC_CHANGE_STATE(FOC_IDLE);
}
void protection_manager_run()
{
    if (g_protection_manager.fault_flag)
        return;
    // 错误
    if (get_motor_fault_flag())
    {
        g_protection_manager.fault = MOTOR_FAULT;
        g_protection_manager.fault_flag = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }

    if (g_monitor.Iu > MAX_Current || g_monitor.Iv > MAX_Current || g_monitor.Iw > MAX_Current || Tolerance_check(&g_monitor.iq_fb, g_protection_manager.maxcurrent, 0, g_protection_manager.tolerance_current))
    {
        g_protection_manager.fault = OVER_CURRENT;
        g_protection_manager.fault_flag = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    if (CAN_STATE_get() != 0)
    {
        g_protection_manager.fault = CAN_STATE_get() - 1 + CAN_INIT_FAULT;
        g_protection_manager.fault_flag = true;
    }
    if (Tolerance_check(&g_adaptive_con.Udc, MAX_Voltage, MIN_Voltage, g_protection_manager.tolerance_voltage) && g_adaptive_con.Udc > MAX_Voltage)
    {
        g_protection_manager.fault = OVER_VOLTAGE;
        g_protection_manager.fault_flag = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    //    if (g_adaptive_con.Udc < MIN_Voltage && Tolerance_check(&g_adaptive_con.Udc, MAX_Voltage, MIN_Voltage, g_protection_manager.tolerance_voltage))
    //    {
    //        g_protection_manager.fault = UNDER_VOLTAGE;
    //        g_protection_manager.fault_flag = true;
    //        FOC_CHANGE_STATE(FOC_FAULT);
    //    }

    // 警告
    if (g_adaptive_con.tempareture > MAX_Temperature)
    {
        g_protection_manager.warning = OVER_TEMPERATURE;
        g_protection_manager.warning_flag = true;
        FOC_CHANGE_STATE(FOC_FAULT);
    }
    else if (g_protection_manager.warning == OVER_TEMPERATURE)
    {
        g_protection_manager.warning_flag = false;
        g_protection_manager.warning = NO_WARNING;
        g_protection_manager.log_done = false;
        FOC_CHANGE_STATE(FOC_IDLE);
    }
    if (g_foccore.run_mode == ENCODER_CONTROL)
    { // 有感模式启动编码器判断
        u8 encoder_state = GET_ENCODER_STATUS();
        if (encoder_state != 0)
        {
            g_protection_manager.warning = ENCODER_OFFLINE - 1 + encoder_state;
            g_protection_manager.warning_flag = true;
            FOC_CHANGE_STATE(FOC_FAULT);
        }
        else if (g_protection_manager.warning >= ENCODER_OFFLINE || g_protection_manager.warning <= ENCODER_WEAK_MAG)
        {
            g_protection_manager.warning_flag = false;
            g_protection_manager.warning = NO_WARNING;
            g_protection_manager.log_done = false;
            FOC_CHANGE_STATE(FOC_IDLE);
        }
    }
    if ((g_protection_manager.warning_flag || g_protection_manager.fault_flag) && !g_protection_manager.log_done)
    {
        if (USB_Connect_Status_get() == 1)
            return;
        log_write();
        g_protection_manager.log_done = true;
    }
}

fault_e GET_Protect_fault()
{
    return g_protection_manager.fault;
}