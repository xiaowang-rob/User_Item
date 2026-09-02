#ifndef __ENCODER_DRIVERS_H
#define __ENCODER_DRIVERS_H

#include "usr/abs/encoder.h"
#include "usr/if/spi_if.h"
#include "usr/if/time_if.h"

// ============================================================
// encoder_drivers.h — 编码器芯片驱动统一出口（usr/drv）
//
// 供组装层使用：选芯片 ops → create(注入总线+时间) → 交给 abs 业务对象。
// 每个芯片驱动只依赖注入的 usr/if 接口表，不感知任何板级资源。
// ============================================================

// ---- AS5047（SPI Mode1 / 16bit / 14bit 分辨率 16384） ----
EncoderChipHandle AS5047_create(const tSpiBusIf *bus, const tTimeIf *time);
void AS5047_destroy(EncoderChipHandle h);
extern const tEncoderDriverOps AS5047_driver_ops;

// ---- MT6816（SPI Mode3 / 16bit / 14bit 分辨率 16384） ----
EncoderChipHandle MT6816_create(const tSpiBusIf *bus, const tTimeIf *time);
void MT6816_destroy(EncoderChipHandle h);
extern const tEncoderDriverOps MT6816_driver_ops;

// ---- MT6835（SPI Mode3 / 8bit / 14bit 分辨率 16384） ----
EncoderChipHandle MT6835_create(const tSpiBusIf *bus, const tTimeIf *time);
void MT6835_destroy(EncoderChipHandle h);
extern const tEncoderDriverOps MT6835_driver_ops;

#endif // __ENCODER_DRIVERS_H
