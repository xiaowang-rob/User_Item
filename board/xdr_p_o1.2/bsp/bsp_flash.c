//
// ******************************************************************************
// STM32F4xx MCU 内置 Flash 驱动
// 提供擦除、编程、读取、升级标志管理、跳转等功能
// 需在 config.h 中定义：APP_START_ADDR, IAP_FLAG_ADDRESS, CONFIG_SECTOR
// ******************************************************************************
//

#include "stm32f4xx_hal.h"
#include "bsp_flash.h"
#include <string.h>
#include "board_config.h"

// 私有宏 ------------------------------------------------------------------
#define FLASH_SECTOR_16KB (16 * 1024)
#define FLASH_SECTOR_64KB (64 * 1024)
#define FLASH_SECTOR_128KB (128 * 1024)

#define FLASH_SECTOR_NUM 12 // Flash 总扇区数量
// STM32F405 扇区起始地址表（共12个扇区）
static const u32 sector_start_addr[] = {
    0x08000000, // Sector 0   16KB
    0x08004000, // Sector 1   16KB
    0x08008000, // Sector 2   16KB
    0x0800C000, // Sector 3   16KB
    0x08010000, // Sector 4   64KB
    0x08020000, // Sector 5  128KB
    0x08040000, // Sector 6  128KB
    0x08060000, // Sector 7  128KB
    0x08080000, // Sector 8  128KB
    0x080A0000, // Sector 9  128KB
    0x080C0000, // Sector 10 128KB
    0x080E0000, // Sector 11 128KB
    0x08100000  // Flash 结束地址（1MB 容量）
};
// 地址分区配置
#define SECTOR_BL_NUM 2  // Bootloader 占用的扇区数量
#define SECTOR_APP_NUM 7 // App 占用的扇区数量
#define SECTOR_USR_NUM 3 // 用户数据占用的扇区数量

const uint8_t sector_bl[SECTOR_BL_NUM] = {0, 1};
const uint8_t sector_app[SECTOR_APP_NUM] = {2, 3, 4, 5, 6, 7, 8};
const uint8_t sector_usr[SECTOR_USR_NUM] = {9, 10, 11};

// 地址预计算
const uint32_t bl_start_addr = sector_start_addr[sector_bl[0]];
const uint32_t bl_end_addr = sector_start_addr[sector_bl[SECTOR_BL_NUM - 1] + 1] - 1;
const uint32_t app_start_addr = sector_start_addr[sector_app[0]];
const uint32_t app_end_addr = sector_start_addr[sector_app[SECTOR_APP_NUM - 1] + 1] - 1;
const uint32_t usr_start_addr = sector_start_addr[sector_usr[0]];
const uint32_t usr_end_addr = sector_start_addr[sector_usr[SECTOR_USR_NUM - 1] + 1] - 1;

const uint32_t bl_size = bl_end_addr - bl_start_addr + 1;
const uint32_t app_size = app_end_addr - app_start_addr + 1;
const uint32_t usr_size = usr_end_addr - usr_start_addr + 1;

// 等待 Flash 就绪并清除错误标志
static bool Flash_WaitReady(u32 timeout_ms)
{
    while ((__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET) && (timeout_ms-- > 0))
        __NOP();
    if (timeout_ms == 0)
        return false;

    return true;
}

// return 扇区号地址
// num_sectors 返回扇区数量
uint8_t *bsp_flash_get_bl_config(uint8_t *num_sectors)
{
    if (num_sectors)
        *num_sectors = SECTOR_BL_NUM;
    return sector_bl;
}
uint8_t *bsp_flash_get_app_config(uint8_t *num_sectors)
{
    if (num_sectors)
        *num_sectors = SECTOR_APP_NUM;
    return sector_app;
}
uint8_t *bsp_flash_get_usr_config(uint8_t *num_sectors)
{
    if (num_sectors)
        *num_sectors = SECTOR_USR_NUM;
    return sector_usr;
}
uint32_t bsp_flash_get_sector_size(u8 sector_idx)
{
    if (0 > sector_idx || FLASH_SECTOR_NUM <= sector_idx)
        return 0;
    return sector_start_addr[sector_idx + 1] - sector_start_addr[sector_idx];
}

uint32_t bsp_flash_get_sector_start_addr(u8 sector_idx)
{
    if (0 > sector_idx || FLASH_SECTOR_NUM <= sector_idx)
        return NULL;
    return sector_start_addr[sector_idx]; // 返回扇区起始地址
}

// 擦除指定的扇区（通过扇区号）
// sector_num 扇区号（0~11）
// true 成功
bool bsp_flash_erase_sector(u8 sector_idx)
{
    if (sector_idx >= FLASH_SECTOR_NUM)
        return false;

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
        .Sector = sector_idx,
        .NbSectors = 1};
    u32 sector_error;
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &sector_error);
    HAL_FLASH_Lock();

    // 等待 BSY 清零并清除可能的错误
    Flash_WaitReady(100);
    return (status == HAL_OK);
}

// 读取 Flash 数据
// pBuffer 输出缓冲区
// ReadAddr 起始地址
// NumByteToRead 读取字节数
// true 成功
bool bsp_flash_read_data(u32 ReadAddr, u8 *pBuffer, u16 NumByteToRead)
{
    if (!pBuffer || ReadAddr < sector_start_addr[0] || (ReadAddr + NumByteToRead) > sector_start_addr[FLASH_SECTOR_NUM])
        return false;
    memcpy(pBuffer, (u8 *)ReadAddr, NumByteToRead);
    return true;
}

// 以字节为单位写入 Flash 数据
bool bsp_flash_write_data(const u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    if (!pBuffer || WriteAddr < sector_start_addr[0] || (WriteAddr + NumByteToWrite) > sector_start_addr[FLASH_SECTOR_NUM])
        return false;
    HAL_FLASH_Unlock();
    for (u16 i = 0; i < NumByteToWrite; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, WriteAddr, pBuffer[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
        WriteAddr++;
    }
    HAL_FLASH_Lock();
    return true;
}

// 以字（32位）为单位写入 Flash（会自动调整）
// pBuffer 数据缓冲区（至少 NumByteToWrite 字节）
// WriteAddr 起始地址
// NumByteToWrite 写入字节数
// true 成功
bool bsp_flash_write_word(u32 WriteAddr, const u8 *pBuffer, u16 NumByteToWrite)
{
    if (!pBuffer || WriteAddr < sector_start_addr[0] || (WriteAddr + NumByteToWrite) > sector_start_addr[FLASH_SECTOR_NUM])
        return false;

    HAL_FLASH_Unlock();
    u16 i = 0;

    // 先按字（4字节）写入
    for (; i + 3 < NumByteToWrite; i += 4)
    {
        u32 word = pBuffer[i] | (pBuffer[i + 1] << 8) | (pBuffer[i + 2] << 16) | (pBuffer[i + 3] << 24);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, WriteAddr + i, word) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
    }

    // 处理剩余不足4字节（按字节写入，但注意 STM32F4 的字节编程要求半字对齐？实际支持字节，但效率低）
    while (i < NumByteToWrite)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, WriteAddr + i, pBuffer[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
        i++;
    }

    HAL_FLASH_Lock();
    return true;
}

// 跳转到指定地址执行
bool bsp_jump_addr(uint32_t addr)
{
    u32 app_stack = *(__IO u32 *)addr;
    u32 app_reset = *(__IO u32 *)(addr + 4);

    // 简单检查栈指针是否有效（通常栈顶应在 SRAM 范围内）
    if (app_stack >= 0x20000000 && app_stack <= 0x20020000)
    {
        __disable_irq();
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;

        __set_MSP(app_stack);
        ((void (*)(void))app_reset)(); // 跳转，不会返回
    }
    return false;
}
