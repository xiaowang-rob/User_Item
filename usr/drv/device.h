#ifndef __DEVICE_H
#define __DEVICE_H

#include "encoder.h"
#include "flash_abs.h"
#include "led.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
// 设备状态
typedef enum
{
    OFFLINE,
    ONLINE,
    RUN_ERROR,
    RUNNING,
} eDeviceStatus;

// 编码器相关驱动
EncoderChipHandle MT6816_create(void);
void MT6816_destroy(EncoderChipHandle handle);

extern tEncoderDriverOps MT6816_driver_ops;

EncoderChipHandle MT6835_create(void);
void MT6835_destroy(EncoderChipHandle handle);

extern tEncoderDriverOps MT6835_driver_ops;

EncoderChipHandle AS5047_create(void);
void AS5047_destroy(EncoderChipHandle handle);

extern tEncoderDriverOps AS5047_driver_ops;
// flash相关定义

#endif