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
bool Tolerance_check(float value, float max_value, float min_value, float tolerance)
{
    if (value > max_value * tolerance || value < min_value * tolerance)
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
    g_pro_manager.com_state = com_state_get_adr();
    g_pro_manager.drive_state = drive_state_get_adr();
}
// 保护程序复位
void protection_manager_reset()
{
    g_pro_manager.fault_flag = false;
    g_pro_manager.warning_flag = false;
    g_pro_manager.fault = NO_FAULT;
    g_pro_manager.warning = NO_WARNING;
    fFOC_Init();
}
void clear_warning_flag(Warning_e warning)
{
    if (g_pro_manager.warning == warning)
    {
        g_pro_manager.warning_flag = false;
        g_pro_manager.warning = NO_WARNING;
    }
}
void protection_manager_run()
{
    // 采集数据
    ADC2_sample(); // 采集电压和温度
    ADC_GET_Temp(&g_pro_manager.temp_u, &g_pro_manager.temp_v, &g_pro_manager.temp_w, &g_pro_manager.temperature);

    if (g_pro_manager.fault_flag)
        return;
    // A监管保护
    // 错误：
    // 1.整定
    if (g_foc.tun->fault_flag)
    {
        switch (g_foc.tun->fault_type)
        {
        case PARAM_FAULT_TIMEOUT:
            g_pro_manager.fault = TUNING_TIMEOUT;
            break;
        case PARAM_FAULT_POLE_PAIRS_MISMATCH:
            g_pro_manager.fault = POLE_PAIRS_MISMATCH;
            break;
        default:
            g_pro_manager.fault = MOTOR_PARAM_FAULT;
            break;
        }
        g_pro_manager.fault_flag = true;
    }
    // 2.电压异常
    if (Tolerance_check(g_foc.motor->Udc, MAX_Voltage, MIN_Voltage, g_pro_manager.tolerance_voltage))
    {
        if (g_foc.motor->Udc > MAX_Voltage)
            g_pro_manager.fault = OVER_VOLTAGE;
        else
            g_pro_manager.fault = LOW_VOLTAGE;
        g_pro_manager.fault_flag = true;
    }
    // 3.电流过大
    if (g_foc.val->Iu > MAX_Current || g_foc.val->Iv > MAX_Current || g_foc.val->Iw > MAX_Current ||
        Tolerance_check(g_foc.val->iq_fb, g_pro_manager.maxcurrent, 0, g_pro_manager.tolerance_current))
    {
        g_pro_manager.fault = OVER_CURRENT;
        g_pro_manager.fault_flag = true;
    }

    // 4.CAN通讯异常
    if (g_pro_manager.com_state->can_state != ONLINE)
    {
        if (g_pro_manager.com_state->can_state == RUN_ERROR)
        {
            g_pro_manager.fault = CAN_COMMUNICATION_FAULT;
        }
        else
        {
            g_pro_manager.fault = CAN_INIT_FAULT;
        }
        g_pro_manager.fault_flag = true;
    }
    // 警告：
    // 1温度过高
    if (g_pro_manager.temperature > MAX_Temperature)
    {
        g_pro_manager.warning = OVER_TEMPERATURE;
        g_pro_manager.warning_flag = true;
    }
    else
        clear_warning_flag(OVER_TEMPERATURE);
    // 2 速度检测
    if (Tolerance_check(g_foc.val->omega_fb, g_pro_manager.maxomega, -g_pro_manager.maxomega, g_pro_manager.tolerance_speed))
    {
        g_pro_manager.warning = OVER_SPEED;
        g_pro_manager.warning_flag = true;
    }
    else
        clear_warning_flag(OVER_SPEED);
    // 3位置检测
    if (Tolerance_check(g_foc.val->pos_fb, g_pro_manager.maxposition, g_pro_manager.minposition, g_pro_manager.tolerance_position))
    {
        g_pro_manager.warning = OVER_POSITION;
        g_pro_manager.warning_flag = true;
    }
    else
        clear_warning_flag(OVER_POSITION);
    //  4编码器状态检测
    if (g_foc.mode->run_mode == ENCODER_CONTROL)
    { // 有感模式启动编码器判断
        if (g_pro_manager.drive_state->ENCODER_state != ONLINE)
            switch (g_pro_manager.drive_state->ENCODER_state)
            {
                g_pro_manager.warning_flag = true;
            case RUN_ERROR:
                g_pro_manager.warning = ENCODER_COM_ERROR;
                break;
            case SINGNAL_ERROR:
                g_pro_manager.warning = ENCODER_WEAK_MAG;
                break;
            default: // 初始化失败
                g_pro_manager.warning = ENCODER_OFFLINE;
                break;
            }
        else
        {
            clear_warning_flag(ENCODER_OFFLINE);
            clear_warning_flag(ENCODER_COM_ERROR);
            clear_warning_flag(ENCODER_WEAK_MAG);
        }
    }
    //   B错误处理
    if ((g_pro_manager.warning_flag || g_pro_manager.fault_flag) && !g_pro_manager.log_done)
    {
        log_data_save();
        FOC_CHANGE_STATE(FOC_FAULT);
        if (g_pro_manager.com_state->Host_port != NONE_port)
            return; // 上位机模式下不进行日志写入
        log_data_write();
        g_pro_manager.log_done = true;
    }
}
