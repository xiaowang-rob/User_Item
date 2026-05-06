
#include "protection_manager.h"
#include "foc_core.h"
#include "usr_config.h"
#include "foc_main.h"
#include "log.h"
#include "parameter_manager.h"
#include "bsp_adc.h"

tDeviceStatus g_device_status = {.encoder_state = ONLINE};
tProtectionManager g_pro_manager = {.com_state = &g_com_state, .drive_state = &g_device_status};
// 容忍度检测
static bool _ToleranceCheck(float value, float max_value, float min_value)
{
    if (value > max_value * g_pro_manager.tolerance_limit || value < min_value / g_pro_manager.tolerance_limit)
        return true;
    return false;
}

void fProManagerInit()
{
    g_pro_manager.fault = NO_FAULT;
    g_pro_manager.warning = NO_WARNING;
    g_pro_manager.fault_flag = false;
    g_pro_manager.warning_flag = false;
    g_pro_manager.maxcurrent = g_Param.limit_current;
    g_pro_manager.maxomega = g_Param.limit_omega;
    g_pro_manager.minposition = g_Param.limit_position_min;
    g_pro_manager.maxposition = g_Param.limit_position_max;
    g_pro_manager.tolerance_time_ms = g_Param.tolerance_time;
    g_pro_manager.tolerance_limit = g_Param.tolerance_limit;
}
void fProSetLimitPosition(float min_position, float max_position)
{
    g_pro_manager.minposition = min_position;
    g_pro_manager.maxposition = max_position;
}
// 保护程序复位
void fProManagerReset()
{
    g_pro_manager.fault_flag = false;
    g_pro_manager.warning_flag = false;
    g_pro_manager.fault = NO_FAULT;
    g_pro_manager.warning = NO_WARNING;
    fFOC_StateUpdate(FOC_RESET);
}
static u32 sample_time_prev_ms = 0;
// 保护主循环
void fProManagerMainLoop()
{
    // 采集数据
    if (BSP_GetTick() - sample_time_prev_ms > TEMP_VBUS_TS_MS)
    {
        sample_time_prev_ms = BSP_GetTick();
        BSP_TempVbusSample(); // 采集电压和温度
        BSP_AdcGetTemp(&g_pro_manager.temperature);
    }

    if (g_pro_manager.fault_flag)
        return;

    // 驱动状态--flash一定得是ONLINE
    if (g_pro_manager.drive_state->flash_state != ONLINE)
    {
        g_pro_manager.fault = FLASH_OFFLINE;
        g_pro_manager.fault_flag = true;
    }

    // 过压不可取
    if (g_foc.core->motor->Udc > MAX_VOLTAGE)
    {
        g_pro_manager.fault = OVER_VOLTAGE;
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
    // 失能保护
    if (!g_foc.foc_enable)
    {

        // 3.电流过大
        if (g_foc.core->foc_val->Iu > MAX_CURRENT || g_foc.core->foc_val->Iv > MAX_CURRENT || g_foc.core->foc_val->Iw > MAX_CURRENT ||
            _ToleranceCheck(g_foc.core->foc_val->iq_fb, g_pro_manager.maxcurrent, -g_pro_manager.maxcurrent))
        {
            g_pro_manager.fault = OVER_CURRENT;
            g_pro_manager.fault_flag = true;
        }

        // 警告：
        // 1温度过高
        if (g_pro_manager.temperature > MAX_TEMPERATURE)
        {
            g_pro_manager.warning = OVER_TEMPERATURE;
            g_pro_manager.warning_flag = true;
        }
    }
    else // 使能保护
    {
        // 1.整定
        if (g_foc.tun->fault != TUNE_FAULT_NONE)
        {
            switch (g_foc.tun->fault)
            {
                // todo:还有很多错误待扩充
            case TUNE_FAULT_CURRENT_VIBRATION:
                g_pro_manager.fault = TUNING_CURRENT_VIBRATION;
                break;
            case TUNE_FAULT_POLEPAIRS_MISMATCH:
                g_pro_manager.fault = TUNING_POLEPAIRS_MISMATCH;
                break;
            case TUNE_FAULT_MECH_LOCKED:
                g_pro_manager.fault = TUNING_MOTOR_LOCKED;
                break;
            default:
                g_pro_manager.fault = TUNING_RSLS_FAULT + g_foc.tun->fault - 4;
                break;
            }
            g_pro_manager.fault_flag = true;
        }
        // 2.电压异常
        if (_ToleranceCheck(g_foc.core->motor->Udc, MAX_VOLTAGE, MIN_VOLTAGE))
        {
            if (g_foc.core->motor->Udc > MAX_VOLTAGE)
            {
                g_pro_manager.fault = OVER_VOLTAGE;
                g_pro_manager.fault_flag = true;
            }
            else
            {
                //                g_pro_manager.fault = LOW_VOLTAGE;
                //                g_pro_manager.fault_flag = true;
            }
        }
        // 3.电流过大
        if (g_foc.core->foc_val->Iu > MAX_CURRENT || g_foc.core->foc_val->Iv > MAX_CURRENT || g_foc.core->foc_val->Iw > MAX_CURRENT ||
            _ToleranceCheck(g_foc.core->foc_val->iq_fb, g_pro_manager.maxcurrent, -g_pro_manager.maxcurrent))
        {
            g_pro_manager.fault = OVER_CURRENT;
            g_pro_manager.fault_flag = true;
        }
        // 1温度过高
        if (g_pro_manager.temperature > MAX_TEMPERATURE)
        {
            g_pro_manager.warning = OVER_TEMPERATURE;
            g_pro_manager.warning_flag = true;
        }

        // 2 速度检测
        if (_ToleranceCheck(g_foc.core->foc_val->rpm_fb, g_pro_manager.maxomega, -g_pro_manager.maxomega))
        {
            g_pro_manager.warning = OVER_SPEED;
            g_pro_manager.warning_flag = true;
        }
        // 3位置检测 位置模式下监测
        if (g_foc.core->foc_mode->runmode == POSITION_MODE)
        {
            if (_ToleranceCheck(g_foc.core->foc_val->pos_fb, g_pro_manager.maxposition, g_pro_manager.minposition))
            {
                g_pro_manager.warning = OVER_POSITION;
                g_pro_manager.warning_flag = true;
            }
        }
    }

    //   B错误处理--日志模块还得优化
    if (g_pro_manager.fault_flag || g_pro_manager.warning_flag)
    {
        fLogDataSave();
        fFOC_StateUpdate(FOC_FAULT);
        fLogDataWrite();
    }
}
