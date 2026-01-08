#include "log.h"
#include "flashDr.h"
#include "protection_manager.h"
#include "string.h"
#include "foc_core.h"
#include "math_fast.h"
Index_t Index;
LOG_t Log;
void log_init(void)
{
    FLASH_Read_data((u8 *)&Index, Index_start_addr, sizeof(Index));
    if (Index.num == 0xff)
    { // 被擦除过
        Index.num = 0;
        Index.block_erase_num = 0;
        Index.write_addr = Log_start_addr;
    }
    if (Index.num == 9)
        log_erase();
}
void log_data_save(void)
{
    Log.Vbus = g_adaptive_con.Udc;
    Log.TEMP = g_adaptive_con.tempareture;
    Log.Iu = g_monitor.Iu;
    Log.Iv = g_monitor.Iv;
    Log.Iw = g_monitor.Iw;
    Log.Iq = g_monitor.iq_fb;
    Log.Id = g_monitor.id_fb;
    Log.Id_ref = g_foccore.id_ref;
    Log.Iq_ref = g_foccore.iq_ref;
    Log.speed = rad_to_rpm(g_monitor.omega_fb);
    Log.speed_ref = rad_to_rpm(g_foccore.omega_ref);
    Log.position = rad_to_deg(g_monitor.pos_fb);
    Log.position_ref = rad_to_deg(g_foccore.pos_ref);
    Log.loop_mode = g_foccore.loop_mode;
    Log.run_mode = g_foccore.run_mode;
    Log.fault = g_protection_manager.fault;
    Log.warning = g_protection_manager.warning;
    Log.num = Index.num + 1;
    u32 time = HAL_GetTick();
    Log.minute = time / 1000 / 60;
    Log.seconds = time / 1000 % 60;
}
void log_data_write(void)
{
    FLASH_Write_Word((u8 *)&Log, Index.write_addr, sizeof(LOG_t));
    Index.num++;
    Index.write_addr += sizeof(LOG_t);
    Index.block_erase_num++;
    FLASH_erase_sector(Index_start_addr, sizeof(Index));
    FLASH_Write_Word((u8 *)&Index, Index_start_addr, sizeof(Index));
}
// 全部读取
void log_read(u8 *num, u32 *block_erase_num, u8 *len, u8 *data)
{
    if (Index.num == 0)
        *num = 0;
    else
    {
        *num = Index.num;
        *len = sizeof(LOG_t) * Index.num;
        FLASH_Read_data(data, Log_start_addr, *len);
    }
}

void log_erase()
{
    if (Index.num == 0)
        return;
    FLASH_erase_sector(Log_Sector_start * 0x00001000, (Index.num + 1) * sizeof(LOG_t));
    Index.num = 0;
    Index.block_erase_num++;
    Index.write_addr = Log_start_addr;
    FLASH_erase_sector(0, sizeof(Index));
    FLASH_Write_Word((u8 *)&Index, 0, sizeof(Index));
}