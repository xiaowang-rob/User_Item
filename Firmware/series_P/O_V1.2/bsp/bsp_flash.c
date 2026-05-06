/**
 ******************************************************************************
 * @file    bsp_flash.c
 * @brief   STM32F4xx MCU 内置 Flash 驱动
 * @details 提供擦除、编程、读取、升级标志管理、跳转等功能
 * @note    需在 config.h 中定义：APP_START_ADDR, FLAG_ADDRESS, CONFIG_SECTOR
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"
#include "bsp_flash.h"
#include <string.h>
#include "config.h"

/* 私有宏 ------------------------------------------------------------------*/
#define FLASH_SECTOR_16KB (16 * 1024)
#define FLASH_SECTOR_64KB (64 * 1024)
#define FLASH_SECTOR_128KB (128 * 1024)

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
};

/* 私有函数声明 ------------------------------------------------------------*/
static bool Flash_WaitReady(u32 timeout_ms);
static u8 Flash_GetSectorNum(u32 addr);
static bool Flash_EraseSectorByNum(u8 sector_num);

/* 公有函数 ----------------------------------------------------------------*/

/**
 * @brief 读取 Flash 数据
 * @param pBuffer 输出缓冲区
 * @param ReadAddr 起始地址
 * @param NumByteToRead 读取字节数
 * @retval true 成功
 */
bool BSP_FLASH_ReadData(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
    if (!pBuffer || ReadAddr < FLASH_BASE || (ReadAddr + NumByteToRead) > FLASH_END_ADDR)
        return false;
    memcpy(pBuffer, (u8 *)ReadAddr, NumByteToRead);
    return true;
}

/**
 * @brief 以字（32位）为单位写入 Flash（地址需4字节对齐，长度需4的倍数）
 * @param pBuffer 数据缓冲区（至少 NumByteToWrite 字节）
 * @param WriteAddr 起始地址（4字节对齐）
 * @param NumByteToWrite 写入字节数（4的倍数）
 * @retval true 成功
 */
bool BSP_FLASH_WriteWord(const u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    if ((WriteAddr & 3) || (NumByteToWrite & 3))
        return false; // 未4字节对齐或长度不是4倍数

    u32 *pSrc = (u32 *)pBuffer;
    u32 addr = WriteAddr;
    u16 cnt = NumByteToWrite / 4;

    HAL_FLASH_Unlock();
    for (u16 i = 0; i < cnt; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, pSrc[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
        addr += 4;
    }
    HAL_FLASH_Lock();
    return true;
}

/**
 * @brief 擦除指定地址范围涉及的所有扇区（地址边界自动对齐到扇区）
 * @param start_addr 起始地址（包含）
 * @param end_addr   结束地址（包含）
 * @retval true 成功
 */
bool BSP_FLASH_EraseRange(u32 start_addr, u32 end_addr)
{
    if (start_addr > end_addr)
        return false;

    u8 start_sector = Flash_GetSectorNum(start_addr);
    u8 end_sector = Flash_GetSectorNum(end_addr);
    if (start_sector == 0xFF || end_sector == 0xFF)
        return false;

    // 保护 Bootloader 区域（根据 BL_SIZE_KB 动态计算占用的扇区数量）
    u8 bl_sectors = (BL_SIZE_KB * 1024 + FLASH_SECTOR_16KB - 1) / FLASH_SECTOR_16KB; // 向上取整
    if (start_sector < bl_sectors)
        start_sector = bl_sectors; // 至少从 Bootloader 后面的扇区开始，避开该区域

    // 保护配置扇区（存升级标志）
    u8 cfg_sector = Flash_GetSectorNum(FLAG_ADDRESS);
    if (end_sector >= cfg_sector)
        end_sector = cfg_sector - 1;

    if (start_sector > end_sector)
        return false;

    HAL_FLASH_Unlock();
    for (u8 s = start_sector; s <= end_sector; s++)
    {
        if (!Flash_EraseSectorByNum(s))
        {
            HAL_FLASH_Lock();
            return false;
        }
    }
    HAL_FLASH_Lock();
    return true;
}

/**
 * @brief 擦除整个 App 区（从 APP_START_ADDR 到 FLASH_END_ADDR，避开配置扇区）
 * @note   BSP_Flash_CalcEraseSectors 不再需要，直接调用本函数即可
 * @retval true 成功
 */
bool BSP_Flash_EraseApp(void)
{
    return BSP_FLASH_EraseRange(APP_START_ADDR, FLASH_END_ADDR);
}

/**
 * @brief 写入 APP 固件（自动处理非4字节对齐的尾部，但建议以4字节块调用）
 * @param addr 目标地址（必须 >= APP_START_ADDR）
 * @param data 数据指针
 * @param len  数据长度（字节）
 * @retval true 成功
 */
bool BSP_Flash_WriteApp(u32 addr, const u8 *data, u16 len)
{
    if (addr < APP_START_ADDR || addr + len > FLASH_END_ADDR)
        return false;

    HAL_FLASH_Unlock();
    u16 i = 0;

    // 先按字（4字节）写入
    for (; i + 3 < len; i += 4)
    {
        u32 word = data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
    }

    // 处理剩余不足4字节（按字节写入，但注意 STM32F4 的字节编程要求半字对齐？实际支持字节，但效率低）
    while (i < len)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + i, data[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
        i++;
    }

    HAL_FLASH_Lock();
    return true;
}

/**
 * @brief 校验 Flash 内容与给定数据是否一致
 * @param addr 起始地址
 * @param data 数据缓冲区
 * @param len  长度
 * @retval true 一致
 */
bool BSP_Flash_Verify(u32 addr, const u8 *data, u16 len)
{
    for (u16 i = 0; i < len; i++)
    {
        if (*(u8 *)(addr + i) != data[i])
            return false;
    }
    return true;
}

/**
 * @brief 跳转到 App 执行
 * @retval true 跳转成功（实际不会返回）
 */
bool BSP_JumpToApp(void)
{
    u32 app_stack = *(__IO u32 *)APP_START_ADDR;
    u32 app_reset = *(__IO u32 *)(APP_START_ADDR + 4);

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

/**
 * @brief 设置升级标志（写入版本字符串到标志区）
 * @param firm_version 版本字符串
 * @param version_len  字符串长度（不含终止符）
 * @retval true 成功
 */
bool BSP_JumpToBootloader(const u8 *firm_version, u16 version_len)
{
    // 限制字符串长度不超过 23 字符（留一个字节给 '\0'）
    if (version_len > 23)
        version_len = 23;
    u8 cfg_sector = Flash_GetSectorNum(FLAG_ADDRESS);
    // 先擦除配置扇区
    if (!Flash_EraseSectorByNum(cfg_sector))
        return false;

    // 写入版本字符串（按字写，不足字的部分留空，填充 0）
    u8 buf[24] = {0};
    memcpy(buf, firm_version, version_len);
    // 确保以 '\0' 结尾（便于作为字符串读取）
    buf[version_len] = '\0';

    return BSP_FLASH_WriteWord(buf, FLAG_ADDRESS, 24);
}

/**
 * @brief 获取升级标志（读取版本字符串）
 * @param firm_version 输出缓冲区（至少 24 字节）
 * @param version_len  输出实际长度
 * @retval true 存在有效的升级标志
 */
bool BSP_GetUpgradeFlag(u8 *firm_version, u16 *version_len)
{
    u8 raw[24];
    if (!BSP_FLASH_ReadData(raw, FLAG_ADDRESS, 24))
        return false;

    // 判断是否全为 0xFF（未编程）或全为 0
    bool all_ff = true, all_00 = true;
    for (int i = 0; i < 24; i++)
    {
        if (raw[i] != 0xFF)
            all_ff = false;
        if (raw[i] != 0x00)
            all_00 = false;
    }
    if (all_ff || all_00)
        return false;

    // 复制到输出，确保以 '\0' 结尾
    memcpy(firm_version, raw, 24);
    firm_version[23] = '\0';
    *version_len = strlen((char *)firm_version);
    return (*version_len > 0);
}

/**
 * @brief 清除升级标志（擦除配置扇区）
 * @retval true 成功
 */
bool BSP_ClearUpgradeFlag(void)
{
    u8 cfg_sector = Flash_GetSectorNum(FLAG_ADDRESS);
    return Flash_EraseSectorByNum(cfg_sector);
}

/* 私有函数实现 ------------------------------------------------------------*/

/**
 * @brief 等待 Flash 就绪并清除错误标志
 */
static bool Flash_WaitReady(u32 timeout_ms)
{
    while ((__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET) && (timeout_ms-- > 0))
        __NOP();
    if (timeout_ms == 0)
        return false;

    return true;
}

/**
 * @brief 根据 Flash 地址获取扇区编号（0~11）
 * @param addr Flash 地址
 * @retval 扇区编号，无效返回 0xFF
 */
static u8 Flash_GetSectorNum(u32 addr)
{
    for (u8 i = 0; i < 12; i++)
    {
        if (addr >= sector_start_addr[i])
        {
            if (i == 11)
                return 11;
            if (addr < sector_start_addr[i + 1])
                return i;
        }
    }
    return 0xFF;
}

/**
 * @brief 擦除指定的扇区（通过扇区号）
 * @param sector_num 扇区号（0~11）
 * @retval true 成功
 */
static bool Flash_EraseSectorByNum(u8 sector_num)
{
    if (sector_num > 11)
        return false;

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
        .Sector = sector_num,
        .NbSectors = 1};
    u32 sector_error;
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &sector_error);
    HAL_FLASH_Lock();

    // 等待 BSY 清零并清除可能的错误
    Flash_WaitReady(100);
    return (status == HAL_OK);
}