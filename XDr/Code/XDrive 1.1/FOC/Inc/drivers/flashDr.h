#ifndef __FLASH_DR_H
#define __FLASH_DR_H
#include "main.h"


// 0x00 00 00 00
#define FLASH_SIZE 128 / 8 * 1024 * 1024 // 16MB
#define FLASH_PAGE_SIZE 256              // 256字节
#define FLASH_PAGE_NUM 16                // 16页
#define FLASH_SECTOR_NUM 16              // 16个扇区
#define FLASH_BLOCK_NUM 256              // 256块

void FLASH_Init(void);
void Erase_one_Sector(u32 Address);
void FLASH_erase_sector(u32 Address, u32 Write_data_NUM);
bool FLASH_Read_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool FLASH_Write_Word(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);

#endif /* __FLASH_DR_H */