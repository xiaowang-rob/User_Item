#include "App_IAP.h"
#include "drive_parameters.h"
/**
 * @brief 等待 Flash 空闲并清除错误标志（F405 专用）
 */
static HAL_StatusTypeDef _Flash_WaitReady(uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms;

    /* 1. 等待 BSY 清零（BSY 是只读位，不能写清除） */
    while ((__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY) != RESET) && (timeout-- > 0))
    {
        __NOP();
    }
    if (timeout == 0)
        return HAL_TIMEOUT;

    /* 2. 清除错误标志（F405: 排除 RDERR） */
    // 方法 1：直接写 0xFF 清除所有可清除位（推荐）
    FLASH->SR = 0xFF;

    // 方法 2：使用精确标志组合
    // __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_F405);

    return HAL_OK;
}

bool fApp_JumpToBootloader(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error;

    /* 操作前：等待空闲 + 清标志 */
    if (_Flash_WaitReady(100) != HAL_OK)
    {
        return false;
    }

    /* 关中断，防止 Flash 操作序列被打断 */
    __disable_irq();

    /* 1. 解锁 Flash */
    HAL_FLASH_Unlock();

    /* 2. 配置擦除参数 */
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 3.3V 系统
    erase_init.Sector = CONFIG_SECTOR;               // FLASH_SECTOR_11
    erase_init.NbSectors = 1;

    /* 3. 执行擦除 */
    status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if (status != HAL_OK)
    {
        goto exit_cleanup;
    }

    /* 擦除后：再次等待空闲 + 清标志 */
    _Flash_WaitReady(100);

    /* 4. 写入升级标志-用硬件版本号标记 */
    u8 upgrade_flag[24] = Description " " VERSION;
    u32 *p_word = (uint32_t *)upgrade_flag;
    for (uint8_t i = 0; i < 6; i++) // 24 字节 = 6 个 WORD
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   FLAG_ADDRESS + (i * 4),
                                   *p_word++);
        if (status != HAL_OK)
        {
            break; // 写入失败立即退出
        }
    }

exit_cleanup:
    /* 5. 锁定 Flash */
    HAL_FLASH_Lock();

    /* 恢复中断 */
    __enable_irq();

    /* 最终清除 EOP 标志（避免影响后续操作） */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);

    return (status == HAL_OK);
}