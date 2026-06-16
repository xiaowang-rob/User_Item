#include "log.h"
#include "device.h"
#include "protection_manager.h"
#include "string.h"
#include "math_fast.h"
#include "foc_main.h"

tLogindex Index;
tLog Log;

void log_init(void)
{
    flash_read_data((u8 *)&Index, Log_start_addr, sizeof(Index));
    if (Index.num == 0xff)
    { // 日志被擦除过 或 第一次上电
        Index.num = 0;
        Index.log_addr = Log_start_addr;
    }
    else
    {
        Index.log_addr = Log_start_addr + Index.num * sizeof(Log);
    }
}
void log_data_save(tProtectionManager *pro_manager)
{
    Log.num = Index.num;
    Log.minutes = BSP_GetTick() / 1000 / 60;
    Log.vbus = pro_manager->foc_val->udc;
    Log.temp = pro_manager->foc_val->temp;
    Log.iu = pro_manager->foc_val->iu;
    Log.iv = pro_manager->foc_val->iv;
    Log.iw = pro_manager->foc_val->iw;
    Log.iq = pro_manager->foc_val->iq_fb;
    Log.id = pro_manager->foc_val->id_fb;
    Log.id_ref = pro_manager->foc_val->id_ref;
    Log.iq_ref = pro_manager->foc_val->iq_ref;
    Log.speed = pro_manager->foc_val->rpm_fb;
    Log.speed_ref = pro_manager->foc_val->rpm_ref;
    Log.position = pro_manager->foc_val->pos_fb;
    Log.position_ref = pro_manager->foc_val->pos_ref;
    Log.run_mode = pro_manager->foc_mode->run_mode;
    Log.sensor_mode = pro_manager->foc_mode->sensor_mode;
    Log.fault = (eFaultState)pro_manager->fault;
    Log.warning = (eWarningState)pro_manager->warning;

    Log.can_state = pro_manager->drive_state->can_state;
    Log.encoder_state = pro_manager->drive_state->encoder_state;
}

void log_data_write(void)
{
    flash_write_word((u8 *)&Log, Index.log_addr, sizeof(Log));
    if (Index.num >= MAX_log_NUM)
    { // 日志满了后 循环覆盖最后一条日志
        return;
    }
    Index.num++;
    Index.log_addr += sizeof(Log);
    return;
}

static u8 read_index = 0;
bool log_read_flash(u8 *data, u8 *len)
{
    if (read_index < MAX_log_NUM)
    {
        flash_read_data((u8 *)&Log, Log_start_addr + read_index * sizeof(Log), sizeof(Log));
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

void log_erase()
{
    flash_erase_sector(Log_start_addr, MAX_log_NUM * sizeof(Log));
    flash_erase_sector(Log_Index_start_addr, sizeof(Index));
    Index.num = 0;
    Index.log_addr = Log_start_addr;
}