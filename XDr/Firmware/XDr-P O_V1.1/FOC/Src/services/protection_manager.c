#include "protection_manager.h"
#include "foc_core.h"
#include "drive_parameters.h"
#include "foc_statemachine.h"
#include "log.h"
#include "adc_dr.h"
#include "parameter_manager.h"
#include "system_statemachine.h"

tDeviceStatus g_device_status = {.encoder_state = ONLINE};
tProtectionManager g_pro_manager = {0};
// 容忍度检测
bool _ToleranceCheck(float value, float max_value, float min_value, float tolerance)
{
    if (value > max_value * tolerance || value < min_value / tolerance)
        return true;
    return false;
}

void fProManagerInit()
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
    g_pro_manager.tolerance_time_ms = g_Param.tolerance_time * 1000;
    g_pro_manager.tolerance_voltage = g_Param.tolerance_voltage;
    g_pro_manager.tolerance_current = g_Param.tolerance_current;
    g_pro_manager.tolerance_speed = g_Param.tolerance_speed;
    g_pro_manager.tolerance_position = g_Param.tolerance_position;
    g_pro_manager.com_state = &g_com_state;
    g_pro_manager.drive_state = &g_device_status;
}
// 保护程序复位
void fProManagerReset()
{
    g_pro_manager.fault_flag = false;
    g_pro_manager.warning_flag = false;
    g_pro_manager.fault = NO_FAULT;
    g_pro_manager.warning = NO_WARNING;
    fFOC_Init();
}
// 保护主循环
void fProManagerMainLoop()
{
    // 采集数据
    fAdc2Sample(); // 采集电压和温度
    fAdcGetTemp(&g_pro_manager.temp_u, &g_pro_manager.temp_v, &g_pro_manager.temp_w, &g_pro_manager.temperature);

    if (g_pro_manager.fault_flag)
        return;

    // A监管保护
    // 错误：
    // 驱动状态--flash一定得是ONLINE
    if (g_pro_manager.drive_state->flash_state != ONLINE)
    {
        g_pro_manager.fault = FLASH_OFFLINE;
        g_pro_manager.fault_flag = true;
    }
    // 1.整定
    if (g_foc.tun->fault != TUNE_FAULT_NONE)
    {
        switch (g_foc.tun->fault)
        {
            // todo:还有很多错误待扩充
        case TUNE_FAULT_TIMEOUT:
            g_pro_manager.fault = TUNING_TIMEOUT;
            break;
        case TUNE_FAULT_PARAM_INVALID:
            g_pro_manager.fault = MOTOR_PARAM_INVALID;
            break;
        default:
            g_pro_manager.fault = TUNING_FAULT;
            break;
        }
        g_pro_manager.fault_flag = true;
    }
    // 2.电压异常
    if (_ToleranceCheck(g_foc.core->motor->Udc, MAX_Voltage, MIN_Voltage, g_pro_manager.tolerance_voltage))
    {
        if (g_foc.core->motor->Udc > MAX_Voltage)
            g_pro_manager.fault = OVER_VOLTAGE;
        else
            g_pro_manager.fault = LOW_VOLTAGE;
        g_pro_manager.fault_flag = true;
    }
    // 3.电流过大
    if (g_foc.core->foc_val->Iu > MAX_Current || g_foc.core->foc_val->Iv > MAX_Current || g_foc.core->foc_val->Iw > MAX_Current ||
        _ToleranceCheck(g_foc.core->foc_val->iq_fb, g_pro_manager.maxcurrent, -g_pro_manager.maxcurrent, g_pro_manager.tolerance_current))
    {
        g_pro_manager.fault = OVER_CURRENT;
        g_pro_manager.fault_flag = true;
    }

    // 4.CAN通讯异常
    if (g_pro_manager.drive_state->can_state != ONLINE && g_pro_manager.drive_state->can_state != RUNNING)
    {
        if (g_pro_manager.drive_state->can_state == RUN_ERROR)
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

    // 2 速度检测
    if (_ToleranceCheck(g_foc.core->foc_val->omega_fb, g_pro_manager.maxomega, -g_pro_manager.maxomega, g_pro_manager.tolerance_speed))
    {
        g_pro_manager.warning = OVER_SPEED;
        g_pro_manager.warning_flag = true;
    }
    // 3位置检测 位置模式下监测
    if (g_foc.core->foc_mode->runmode == POSITION_MODE)
    {
        if (_ToleranceCheck(g_foc.core->foc_val->pos_fb, g_pro_manager.maxposition, g_pro_manager.minposition, g_pro_manager.tolerance_position))
        {
            g_pro_manager.warning = OVER_POSITION;
            g_pro_manager.warning_flag = true;
        }
    }
    //  4编码器状态检测
    if (g_foc.core->foc_mode->sensor_mode != SENSORLESS_CONTROL)
    { // 有感模式和混合模式启动编码器判断
        if (g_pro_manager.drive_state->encoder_state != ONLINE && g_pro_manager.drive_state->encoder_state != RUNNING)
        {
            g_pro_manager.warning_flag = true;
            if (g_pro_manager.drive_state->encoder_state == RUN_ERROR)

                g_pro_manager.warning = ENCODER_COM_ERROR;
            else
                g_pro_manager.warning = ENCODER_OFFLINE;
        }
    }
    //   B错误处理--日志模块还得优化
    if (g_pro_manager.fault_flag || g_pro_manager.warning_flag)
    {
        fLogDataSave();
        fFOC_StateUpdate(FOC_FAULT);
        fLogDataWrite();
        g_pro_manager.log_done = true;
    }
}
