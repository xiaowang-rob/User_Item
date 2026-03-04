#include "log.h"
#include "flashDr.h"
#include "protection_manager.h"
#include "string.h"
#include "math_fast.h"
#include "foc_statemachine.h"

/*
只记录每次上电之后所有的报错和警告
*/
tLogindex Index;
tLog Log;

void fLogInit(void)
{
    Index.num = 0;
    for (Index.num = 0; Index.num < MAX_log_NUM; Index.num++)
    {
        fFLASH_ReadData((u8 *)&Log, Log_start_addr + Index.num * sizeof(Log), sizeof(Log));
        if (Log.num != Index.num)
            break;
    }
    Index.log_addr = Log_start_addr + Index.num * sizeof(Log);
}
void fLogDataSave(void)
{
    Log.num = Index.num;
    Log.minutes = HAL_GetTick() / 1000 / 60;
    Log.Vbus = g_foc.core->motor->Udc;
    Log.TEMP = g_pro_manager.temperature;
    Log.Iu = g_foc.core->foc_val->Iu;
    Log.Iv = g_foc.core->foc_val->Iv;
    Log.Iw = g_foc.core->foc_val->Iw;
    Log.Iq = g_foc.core->foc_val->iq_fb;
    Log.Id = g_foc.core->foc_val->id_fb;
    Log.Id_ref = g_foc.core->foc_val->id_ref;
    Log.Iq_ref = g_foc.core->foc_val->iq_ref;
    Log.speed = fRadToRpm(g_foc.core->foc_val->omega_fb);
    Log.speed_ref = fRadToRpm(g_foc.core->foc_val->omega_ref);
    Log.position = fRadToDeg(g_foc.core->foc_val->pos_fb);
    Log.position_ref = fRadToDeg(g_foc.core->foc_val->pos_ref);
    Log.run_mode = g_foc.core->foc_mode->runmode;
    Log.sensor_mode = g_foc.core->foc_mode->sensor_mode;
    Log.fault = (eFault)g_pro_manager.fault;
    Log.warning = (eWarning)g_pro_manager.warning;

    Log.can_state = g_pro_manager.drive_state->can_state;
    Log.encoder_state = g_pro_manager.drive_state->encoder_state;
}
bool fLogDataWrite(void)
{
    fFLASH_WriteWord((u8 *)&Log, Index.log_addr, sizeof(Log));
    Index.num++;
    Index.log_addr += sizeof(Log);
    if (Index.num >= MAX_log_NUM)
        return false; // 日志已满 强制检查日志手动清除
    return true;
}

static u8 read_index = 0;
bool fLogReadFlash(u8 *data, u8 *len)
{
    if (read_index < MAX_log_NUM)
    {
        fFLASH_ReadData((u8 *)&Log, Log_start_addr + read_index * sizeof(Log), sizeof(Log));
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

void fLogErase()
{
    fFLASH_EraseSector(Log_start_addr, MAX_log_NUM * sizeof(Log));
    Index.num = 0;
    Index.log_addr = Log_start_addr;
}