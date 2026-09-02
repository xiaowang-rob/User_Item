#ifndef __FLASH_DRIVERS_H
#define __FLASH_DRIVERS_H

#include "usr/abs/flash.h"
#include "usr/if/spi_if.h"
#include "usr/if/time_if.h"

// ============================================================
// flash_drivers.h — Flash 芯片驱动统一出口（usr/drv）
//
// 供组装层使用：选芯片 ops → create(注入总线+时间) → 交给 abs 磨损/IAP 逻辑。
// ============================================================

// ---- W25Q128（SPI NOR，8bit / 16MB / 4KB 扇区） ----
FlashChipHandle w25qxx_create(const tSpiBusIf *bus, const tTimeIf *time);
void w25qxx_destroy(FlashChipHandle h);
extern const tFlashDriverOps w25qxx_driver_ops;

#endif // __FLASH_DRIVERS_H
