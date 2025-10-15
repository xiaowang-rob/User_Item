#ifndef __ADAPTIVE_CONTROL_H
#define __ADAPTIVE_CONTROL_H
#include "main.h"

typedef struct
{
    u8 temp_u;
    u8 temp_v;
    u8 temp_w;
    float tempareture;
    float Udc;
} ADAPTIVE_CON_T;
extern ADAPTIVE_CON_T g_adaptive_con;

void adaptive_control_init(void);
void adaptive_control_update(void);
#endif