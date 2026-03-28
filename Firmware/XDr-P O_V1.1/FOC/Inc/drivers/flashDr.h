
#ifndef __FLASH_DR_H
#define __FLASH_DR_H

#include "main.h"

/* 存储器规格定义 ---------------------------------------------------------*/
#define FLASH_SIZE (128 / 8 * 1024 * 1024) ///< 总容量：16MB
#define FLASH_PAGE_SIZE 256                ///< 页大小：256字节
#define FLASH_PAGE_NUM 16                  ///< 页数量：16页
#define FLASH_SECTOR_NUM 16                ///< 扇区数量：16个
#define FLASH_BLOCK_NUM 256                ///< 块数量：256块

/* 函数声明 -------------------------------------------------------------*/
void fFLASH_Init(void);
void fEraseOneSector(u32 Address);
void fFLASH_EraseSector(u32 Address, u32 Write_data_NUM);
bool fFLASH_ReadData(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool fFLASH_WriteWord(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);

#endif /* __FLASH_DR_H */