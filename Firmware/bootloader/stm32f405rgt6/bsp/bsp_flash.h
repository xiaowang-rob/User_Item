#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "bsp.h"

bool BSP_FLASH_ReadData(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool BSP_FLASH_WriteWord(const u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
bool BSP_FLASH_EraseRange(u32 start_addr, u32 end_addr);

bool BSP_Flash_EraseApp(void);
bool BSP_Flash_WriteApp(u32 addr, const u8 *data, u16 len);
bool BSP_Flash_Verify(u32 addr, const u8 *data, u16 len);
bool BSP_JumpToApp(void);
bool BSP_JumpToBootloader(const u8 *firm_version, u16 version_len);
bool BSP_GetUpgradeFlag(u8 *firm_version, u16 *version_len);
bool BSP_ClearUpgradeFlag(void);

#endif /* __BSP_FLASH_H */