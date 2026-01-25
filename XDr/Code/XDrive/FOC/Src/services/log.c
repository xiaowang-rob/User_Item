#include "log.h"
#include "flashDr.h"
#include "protection_manager.h"
#include "string.h"
#include "math_fast.h"
#include "foc_statemachine.h"

/*
只记录每次上电之后所有的报错和警告
*/
Index_t Index;
LOG_t Log;
void log_init(void)
{
    Index.num = 0;
    Index.write_addr = Log_start_addr;
    log_erase();
}
void log_data_save(void)
{
    Log.Vbus = g_foc.motor->Udc;
    Log.TEMP = g_pro_manager.temperature;
    Log.Iu = g_foc.val->Iu;
    Log.Iv = g_foc.val->Iv;
    Log.Iw = g_foc.val->Iw;
    Log.Iq = g_foc.val->iq_fb;
    Log.Id = g_foc.val->id_fb;
    Log.Id_ref = g_foc.val->id_ref;
    Log.Iq_ref = g_foc.val->iq_ref;
    Log.speed = rad_to_rpm(g_foc.val->omega_fb);
    Log.speed_ref = rad_to_rpm(g_foc.val->omega_ref);
    Log.position = rad_to_deg(g_foc.val->pos_fb);
    Log.position_ref = rad_to_deg(g_foc.val->pos_ref);
    Log.loop_mode = g_foc.mode->loop_mode;
    Log.run_mode = g_foc.mode->run_mode;
    Log.fault = (fault_e)g_pro_manager.fault;
    Log.warning = (Warning_e)g_pro_manager.warning;

    Log.usb_state = g_pro_manager.com_state->usb_state;
    Log.can_state = g_pro_manager.com_state->can_state;
    Log.flash_state = g_pro_manager.drive_state->FLASH_state;
    Log.encoder_state = g_pro_manager.drive_state->ENCODER_state;
    Log.num = Index.num;
    u32 time = HAL_GetTick();
    Log.seconds = time / 1000;
}
bool log_data_write(void)
{
    FLASH_Write_Word((u8 *)&Log, Index.write_addr, sizeof(Log));
    Index.num++;
    Index.write_addr += sizeof(Log);
    if (Index.num >= MAX_log_NUM)
        return false; // 日志已满 强制检查日志手动清除

    return true;
}

static u8 read_index = 0;
bool log_read_flash(u8 *data, u8 *len)
{
    if (read_index < MAX_log_NUM)
    {
        FLASH_Read_data((u8 *)&Log, Log_start_addr + read_index * sizeof(Log), sizeof(Log));
        if (Log.num == read_index)
        {
            *len = sizeof(Log);
            read_index++;
            memcpy(data, &Log, sizeof(Log));
            return false;
        }
        else
        {
            read_index = 0;
            return true;
        }
    }
    else
    {
        *len = 0;
        read_index = 0;
        return true;
    }
}
void log_read_now(u8 *data, u8 *len)
{
    memcpy(data, &Log, sizeof(Log));
    *len = sizeof(Log);
}

void log_erase()
{
    FLASH_erase_sector(Log_start_addr, MAX_log_NUM * sizeof(Log));
    Index.num = 0;
    Index.write_addr = Log_start_addr;
}