#ifndef __LOG_H
#define __LOG_H

#include "main.h"

#define MAX_log_NUM 9

#define LOG_Block 1
#define Log_Sector_start 0
#define Log_start_addr (u32)(LOG_Block * 0x00010000 + Log_Sector_start * 0x00001000)

typedef struct
{
    u32 log_addr;
    u8 num;
} tLogindex;

typedef struct
{
    u8 num;
    u8 fault;
    u8 warning;

    u8 sensor_mode;
    u8 run_mode;

    u8 can_state;
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
} tLog;

void fLogInit(void);
void fLogDataSave(void);
bool fLogDataWrite(void);
bool fLogReadFlash(u8 *data, u8 *len);
void fLogErase();

#endif /* __LOG_H */