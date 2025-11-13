#ifndef __LOG_H
#define __LOG_H

#include "main.h"

#define LOG_Block 1
#define Index_Sector 0
#define Index_start_addr LOG_Block * 0x00010000 + Index_Sector * 0x00001000
#define Log_Sector_start 1
#define Log_start_addr LOG_Block * 0x00010000 + Log_Sector_start * 0x00001000

typedef struct
{
    u8 num;
    u32 block_erase_num;
    u32 write_addr;
} Index_t;
typedef struct
{
    u8 num;
    u8 minute;
    u8 seconds;
    u8 fault;
    u8 warning;
    u8 FOC_status;
    u8 loop_mode;
    float Vbus;
    float TEMP;
    float Iu;
    float Iv;
    float Iw;
    float Id;
    float Iq;
    float Id_ref;
    float Iq_ref;
    float speed;
    float speed_ref;
    float position;
    float position_ref;
} LOG_t;

void log_init(void);
void log_data_save(void);
void log_data_write(void);
void log_read(u8 *num, u32 *block_erase_num, u8 *len, u8 *data);
void log_erase();
#endif /* __LOG_H */