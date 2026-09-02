#ifndef __HW_FLASH_SPI_H
#define __HW_FLASH_SPI_H

#include "usr/if/spi_if.h"

// ============================================================
// hw_flash_spi.h — 板上外部 Flash 的 SPI 总线资源（board/<b>/hw）
//
// 提供外挂 SPI NOR（如 W25Q128）用的 tSpiBusIf 总线实例：
// SPI2 + CS(PB12)，8bit。set_mode 按驱动声明配置。
// ============================================================

const tSpiBusIf *hw_flash_bus_get(void);

#endif // __HW_FLASH_SPI_H
