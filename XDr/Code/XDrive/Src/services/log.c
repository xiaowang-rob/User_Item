#include "log.h"
#include "flashDr.h"
#include "protection_manager.h"

Index_t Index;
LOG_t Log;
void log_init(void)
{
    Index.num = 0;
    Index.block_erase_num = 0;
    Index.write_addr = Log_start_addr;
    Read_W25Q128_data(&Index, Index_start_addr, sizeof(Index));
    if (Index.num == 256)
        log_erase();
}
void log_write(void)
{
    Log.foc_core_data = g_foccore;
    Log.fault = GET_Protect_fault();
    Log.monitor_data = g_monitor;
    Log.num = Index.num++;
    u32 time = HAL_GetTick();
    Log.minute = time / 1000 / 60;
    Log.seconds = time / 1000 % 60;
    Write_W25Q128_data(&Log, Index.write_addr, sizeof(LOG_t));
    Index.write_addr += sizeof(LOG_t);
    Index.block_erase_num++;

    Erase_Write_data_Sector(0, sizeof(Index));
    Write_W25Q128_data(&Index, 0, sizeof(Index));
}
void log_read(u16 num)
{
    Read_W25Q128_data(&Log, Log_start_addr + num * sizeof(LOG_t), sizeof(LOG_t));
}
void log_erase()
{
    if (Index.num == 0)
        return;
    Erase_Write_data_Sector(Log_Sector_start * 0x00001000, (Index.num + 1) * sizeof(LOG_t));
    Index.num = 0;
    Index.block_erase_num++;
    Index.write_addr = Log_start_addr;
    Erase_Write_data_Sector(0, sizeof(Index));
    Write_W25Q128_data(&Index, 0, sizeof(Index));
}