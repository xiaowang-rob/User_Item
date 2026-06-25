#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "bsp.h"

bool bsp_flash_read_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool bsp_flash_write_word(const u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
bool bsp_flash_erase_range(u32 start_addr, u32 end_addr);
bool bsp_flash_erase_app(void);
bool bsp_flash_write_app(u32 addr, const u8 *data, u16 len);
bool bsp_flash_verify(u32 addr, const u8 *data, u16 len);
bool bsp_jump_to_app(void);
bool bsp_jump_to_bootloader(const u8 *firm_version, u16 version_len);
bool bsp_get_upgrade_flag(u8 *firm_version, u16 *version_len);
bool bsp_clear_upgrade_flag(void);

bool bsp_read_param(u8 *pBuffer, u16 NumByteToRead);
bool bsp_erase_param(void);
bool bsp_write_param(u8 *pBuffer, u16 NumByteToWrite);

bool bsp_write_log(u8 *pBuffer, u8 num, u16 NumByteToWrite);
bool bsp_read_log(u8 *pBuffer, u8 num, u16 NumByteToRead);
bool bsp_erase_log(void);

#endif // __BSP_FLASH_H