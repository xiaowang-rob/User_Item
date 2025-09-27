#ifndef __FLASH_DR_H
#define __FLASH_DR_H

#include "main.h"
#include "base_parameters.h"
#define FLASH_SPI_CS_H() HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, 1)
#define FLASH_SPI_CS_L() HAL_GPIO_WritePin(FLASH_CS_CPIOx, FLASH_CS_CPIOx_PIN, 0)
#define FLASH_SIZE 128 / 8 * 1024 * 1024 // 128Mbit
#define FLASH_PAGE_SIZE 256              // 256字节
#define FLASH_SECTOR_SIZE 4096           // 4K字节
#define FLASH_SECTOR_NUM 128             // 128个扇区

void Read_W25Q128_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
void Write_W25Q128_data(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void W25Q128_Init(void);

#endif /* __FLASH_DR_H */