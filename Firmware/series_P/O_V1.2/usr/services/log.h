#ifndef __LOG_H
#define __LOG_H

#include "bsp.h"

#define MAX_log_NUM 9

#define LOG_Block 1
#define Log_Sector_start 0
#define Log_start_addr (u32)(LOG_Block * 0x00010000 + Log_Sector_start * 0x00001000)

#define Log_Index_Sector_start 2
#define Log_Index_start_addr (u32)(LOG_Block * 0x00010000 + Log_Index_Sector_start * 0x00001000)

typedef struct
{
    u32 log_addr;
    u8 num;
} tLogindex;

typedef struct
{
    u8 num;
    u8 minutes;
    u8 fault;
    u8 warning;

    u8 sensor_mode;
    u8 run_mode;

    u8 can_state;
    u8 encoder_state;

    float vbus;
    float temp;
    float iu;
    float iv;
    float iw;
    float id;
    float iq;
    float id_ref;
    float iq_ref;
    float speed;
    float speed_ref;
    float position;
    float position_ref;
} tLog;

void log_init(void);
void log_data_save(tProtectionManager *pro_manager);
void log_data_write(void);
bool log_read_flash(u8 *data, u8 *len);
void log_erase();

#endif /* __LOG_H */