#include "protection_manager.h"
#include "foc_core.h"
#include "system_parameters.h"
#include "foc_statemachine.h"
#include "log.h"
#include "adcDr.h"
#include "parameter_manager.h"

protection_manager_t g_pro_manager = {0};
u32 _time = 0;
u32 _time_last = 0;
bool Tolerance_check(float *value, float max_value, float min_value, float tolerance)
{
    if (*value > max_value * tolerance || *value < min_value * tolerance)
        _time += (HAL_GetTick() - _time_last);
    else if (_time != 0)
    {
        if (_time > 1)
            _time -= (HAL_GetTick() - _time_last);
        else
            _time = 0;
    }
    if (_time > g_pro_manager.tolerance_time)
        return true;
    _time_last = HAL_GetTick();
    return false;
}

void protection_manager_init()
{
    g_pro_manager.fault = NO_FAULT;
    g_pro_manager.warning = NO_WARNING;
    g_pro_manager.fault_flag = false;
    g_pro_manager.warning_flag = false;
    g_pro_manager.log_done = false;
    g_pro_manager.maxcurrent = g_Param.limit_current;
    g_pro_manager.maxomega = g_Param.limit_omega;
    g_pro_manager.minposition = g_Param.limit_position_min;
    g_pro_manager.maxposition = g_Param.limit_position_max;
    g_pro_manager.tolerance_time = g_Param.tolerance_time;
    g_pro_manager.tolerance_voltage = g_Param.tolerance_voltage;
    g_pro_manager.tolerance_current = g_Param.tolerance_current;
    g_pro_manager.tolerance_speed = g_Param.tolerance_speed;
    g_pro_manager.tolerance_position = g_Param.tolerance_position;
    g_pro_manager.com_state = communication_state_get_adr();
    g_pro_manager.drive_state = drive_state_get_adr();
}
void protection_manager_reset()
{
    g_pro_manager.fault_flag = false;
    g_pro_manager.warning_flag = false;
    FOC_CHANGE_STATE(FOC_RESET);
}
void protection_manager_run()
{
    // 采集数据
    ADC2_sample(); // 采集电压和温度
    ADC_GET_Temp(&g_pro_manager.temp_u, &g_pro_manager.temp_v, &g_pro_manager.temp_w, &g_pro_manager.temperature);

    // 监管保护

    // 错误处理

    if (g_pro_manager.fault_flag)
        return;
    // 错误
    if (get_motor_fault_flag())
    {
        g_pro_manager.fault = MOTOR_FAULT;
        g_pro_manager.fault_flag = true;
    }
    //    if (g_monitor.Iu > MAX_Current || g_monitor.Iv > MAX_Current || g_monitor.Iw > MAX_Current || Tolerance_check(&g_monitor.iq_fb, g_pro_manager.maxcurrent, 0, g_pro_manager.tolerance_current))
    //    {
    //        g_pro_manager.fault = OVER_CURRENT;
    //        g_pro_manager.fault_flag = true;
    //    }
    if (CAN_STATE_get() != 0)
    {
        g_pro_manager.fault = CAN_STATE_get() - 1 + CAN_INIT_FAULT;
        g_pro_manager.fault_flag = true;
    }
    if (Tolerance_check(&g_adaptive_con.Udc, MAX_Voltage, MIN_Voltage, g_pro_manager.tolerance_voltage) && g_adaptive_con.Udc > MAX_Voltage)
    {
        g_pro_manager.fault = OVER_VOLTAGE;
        g_pro_manager.fault_flag = true;
    }
    //    if (g_adaptive_con.Udc < MIN_Voltage && Tolerance_check(&g_adaptive_con.Udc, MAX_Voltage, MIN_Voltage, g_pro_manager.tolerance_voltage))
    //    {
    //        g_pro_manager.fault = UNDER_VOLTAGE;
    //        g_pro_manager.fault_flag = true;
    //    }

    // 警告

    //    if (g_adaptive_con.tempareture > MAX_Temperature)
    //    {
    //        g_pro_manager.warning = OVER_TEMPERATURE;
    //        g_pro_manager.warning_flag = true;
    //    }
    //    else if (g_pro_manager.warning == OVER_TEMPERATURE)
    //    {
    //        g_pro_manager.warning_flag = false;
    //        g_pro_manager.warning = NO_WARNING;
    //        g_pro_manager.log_done = false;
    //        protection_manager_reset();
    //    }
    if (g_foccore.run_mode == ENCODER_CONTROL)
    { // 有感模式启动编码器判断
        u8 encoder_state = GET_ENCODER_STATUS();
        if (encoder_state != 0)
        {
            g_pro_manager.warning = ENCODER_OFFLINE - 1 + encoder_state;
            g_pro_manager.warning_flag = true;
        }
        else if (g_pro_manager.warning >= ENCODER_OFFLINE && g_pro_manager.warning <= ENCODER_WEAK_MAG)
        {
            g_pro_manager.warning_flag = false;
            g_pro_manager.warning = NO_WARNING;
            g_pro_manager.log_done = false;
            protection_manager_reset();
        }
    }

    if ((g_pro_manager.warning_flag || g_pro_manager.fault_flag) && !g_pro_manager.log_done)
    {
        log_data_save();
        FOC_CHANGE_STATE(FOC_FAULT);
        if (USB_Connect_Status_get() == 1)
            return;
        log_data_write();
        g_pro_manager.log_done = true;
    }
}

fault_e GET_Protect_fault()
{
    return g_pro_manager.fault;
}