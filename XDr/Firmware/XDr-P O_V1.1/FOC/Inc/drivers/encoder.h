#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"
#include "device.h"

typedef enum {
    ENCODER_STATE_START_READ,
    ENCODER_STATE_WAIT_HIGH,
    ENCODER_STATE_WAIT_LOW,
    ENCODER_STATE_PROCESS_DATA
} eEncoderState_DMA;

typedef struct {
    eEncoderState_DMA state;
    float angle_abs;
    float angle_last;
    float angle_inc;
    float angle_inc_last;
    float omega;
    u32 last_time;
    int num_turns;
} tEncoder;

#if ENcoder == 1  // MT6816
#define MT6816_REG_ANGLE_HIGH 0x03
#define MT6816_REG_ANGLE_LOW  0x04
#define MT6816_REG_STATUS     0x05
#define MT6816_NO_MAG_WARNING (1 << 1)
#define MT6816_PARITY_CHECK   (1 << 0)
#endif

void fEncoderMainLoopTask(void);
float fGetEncoderAngle_ABS(void);
float fGetEncoderAngle_INC(void);
float fGetEncoderOmega(void);

#endif /* __ENCODER_H */