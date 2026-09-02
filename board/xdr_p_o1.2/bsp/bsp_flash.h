#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "bsp_base.h"

uint8_t *bsp_flash_get_bl_config(uint8_t *num_sectors);
uint8_t *bsp_flash_get_app_config(uint8_t *num_sectors);
uint8_t *bsp_flash_get_usr_config(uint8_t *num_sectors);
uint32_t bsp_flash_get_sector_size(u8 sector_idx);
uint32_t bsp_flash_get_sector_start_addr(u8 sector_idx);
bool bsp_flash_erase_sector(u8 sector_idx);
bool bsp_flash_read_data(u32 ReadAddr, u8 *pBuffer, u16 NumByteToRead);
bool bsp_flash_write_data(const u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
bool bsp_flash_write_word(u32 WriteAddr, const u8 *pBuffer, u16 NumByteToWrite);
bool bsp_jump_addr(uint32_t addr);

#endif // __BSP_FLASH_H