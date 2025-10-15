#ifndef __LOG_H
#define __LOG_H

#include "main.h"
#include "foc_core.h"
#include "loop_control.h"
#define LOG_Block 1
#define Index_Sector 0
#define Index_start_addr LOG_Block * 0x00010000 + Index_Sector * 0x00001000
#define Log_Sector_start 1
#define Log_start_addr LOG_Block * 0x00010000 + Log_Sector_start * 0x00001000

typedef struct
{
    u16 num;
    u32 block_erase_num;
    u32 write_addr;
} Index_t;
typedef struct
{
    u16 num;
    u8 minute;
    u8 seconds;
    u8 fault;
    foc_core_t foc_core_data;
    Monitor_t monitor_data;
} LOG_t;

void log_init(void);
void log_write(void);
void log_read(u16 num);
void log_erase();
#endif /* __LOG_H */