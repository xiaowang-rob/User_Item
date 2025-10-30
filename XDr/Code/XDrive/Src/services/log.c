#include "log.h"
#include "flashDr.h"
#include "protection_manager.h"
#include "string.h"
Index_t Index;
LOG_t Log;
void log_init(void)
{
    FLASH_Read_data((u8 *)&Index, Index_start_addr, sizeof(Index));
    if (Index.num == 0xffff)
    { // 被擦除过
        Index.num = 0;
        Index.block_erase_num = 0;
        Index.write_addr = Log_start_addr;
    }
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
    FLASH_Write_Word((u8 *)&Log, Index.write_addr, sizeof(LOG_t));
    Index.write_addr += sizeof(LOG_t);
    Index.block_erase_num++;

    FLASH_erase_sector(0, sizeof(Index));
    FLASH_Write_Word((u8 *)&Index, 0, sizeof(Index));
}
void log_read(u16 num, u8 *data, u8 *len)
{
    if (num == 0) // 读取最新数据
        num = Index.num;
    FLASH_Read_data((u8 *)&Log, Log_start_addr + num * sizeof(LOG_t), sizeof(LOG_t));
    *len = sizeof(LOG_t);
    memcpy(data, &Log, *len);
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