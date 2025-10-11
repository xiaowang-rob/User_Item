#ifndef __FLASH_DR_H
#define __FLASH_DR_H

#include "base_parameters.h"
#define FLASH_SPI_CS_H() HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, 1)
#define FLASH_SPI_CS_L() HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, 0)

// 0x00 00 00 00
#define FLASH_SIZE 128 / 8 * 1024 * 1024 // 16MB
#define FLASH_PAGE_SIZE 256              // 256字节
#define FLASH_PAGE_NUM 16                // 16页
#define FLASH_SECTOR_NUM 16              // 16个扇区
#define FLASH_BLOCK_NUM 256              // 256块

void Erase_Write_data_Sector(u32 Address, u32 Write_data_NUM);
void Read_W25Q128_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
void Write_W25Q128_data(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void W25Q128_Init(void);

#endif /* __FLASH_DR_H */