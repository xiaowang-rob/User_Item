#ifndef __LOG_H
#define __LOG_H

#include "main.h"

#define MAX_log_NUM 9

#define LOG_Block 1
#define Log_Sector_start 0
#define Log_start_addr (u32)(LOG_Block * 0x00010000 + Log_Sector_start * 0x00001000)

typedef struct
{
    u8 num;
    u32 write_addr;
} Index_t;
typedef struct
{
    u8 num; // 序号
    // 运行时间
    u8 seconds;

    u8 fault;
    u8 warning;

    u8 run_mode;
    u8 loop_mode;

    u8 usb_state;
    u8 can_state;
    u8 flash_state;
    u8 encoder_state;

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
bool log_data_write(void);
bool log_read_flash(u8 *data, u8 *len);
void log_read_now(u8 *data, u8 *len);
void log_erase();
#endif /* __LOG_H */