#include "App_IAP.h"

bool fApp_JumpToBootloader(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error;
    /* 1. 解锁 Flash */
    HAL_FLASH_Unlock();

    /* 2. 擦除标志位所在扇区 (Sector 11) */
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.Sector = CONFIG_SECTOR; // 标志位所在扇区
    erase_init.NbSectors = 1;

    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return false; // 擦除失败
    }

    /* 3. 写入升级标志 */
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLAG_ADDRESS, UPGRADE_MAGIC);

    /* 4. 锁定 Flash */
    HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        return false; // 写入失败
    }
    return true;
}