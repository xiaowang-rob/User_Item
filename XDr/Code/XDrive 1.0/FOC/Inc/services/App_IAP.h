#ifndef __APP_IAP_H
#define __APP_IAP_H

#include "main.h"

/* ========== Flash 地址规划 ========== */
#define APP_START_ADDR 0x08004000U    // App 起始地址 (Sector 1)
#define FLASH_END_ADDR 0x080FFFFFU    // F405 1MB Flash 结束地址
#define CONFIG_SECTOR FLASH_SECTOR_11 // 配置扇区 (存升级标志)，不擦除
                                      /* ========== 固件升级配置 ========== */
                                      /* 修改为 Sector 11 起始地址 (0x080E0000)，确保不会擦除 Bootloader */
                                      /* F405 1MB Flash: Sector 11 地址范围 0x080E0000 - 0x080EFFFF */

#define FLAG_ADDRESS 0x080E0000
#define UPGRADE_MAGIC 0x12345678
#define NORMAL_MAGIC 0xFFFFFFFF

bool fApp_JumpToBootloader(void);

#endif /* __APP_IAP_H */