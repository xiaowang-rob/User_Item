#include "flash_abs.h"
#include <string.h>

// 这一层的扇区和地址传入的是相对地址 调用驱动传出绝对地址和扇区
static inline bool unit_erase(tFlash *flash, tFlashUnit *unit)
{
    // 先擦除
    if (!flash->base_ops->Erase_sector(flash->handle, (uint8_t)flash->usr_sector_id[unit->sector]))
        return false;
    unit->free_addr = 0;
    unit->free_size = unit->size; // 更新空闲空间大小
    return true;                  // 擦除成功
}
// 单元读取
static inline bool unit_read(tFlash *flash, tFlashUnit *unit, uint32_t unit_addr, uint8_t *data, uint16_t size)
{
    return flash->base_ops->Read(flash->handle, data, unit->sector_start_addr + unit_addr, size);
}
// 单元写入-更新空闲地址
static inline bool unit_write(tFlash *flash, tFlashUnit *unit, uint32_t unit_addr, const uint8_t *data, uint16_t size)
{
    bool status = flash->base_ops->Write(flash->handle, data, unit->sector_start_addr + unit_addr, size);
    unit->free_addr += size; // 更新空闲地址
    unit->free_size -= size; // 更新空闲大小
    return status;
}

// 寻找空闲空间起始地址
static inline uint32_t unit_get_free_addr(tFlash *flash, tFlashUnit *unit)
{
    volatile bool found = false;
    uint8_t tmp_data[2] = {0, 0};
    uint32_t tmp_addr_front = 0;
    uint32_t tmp_addr_back = unit->size - 2;

    uint32_t tmp_addr = tmp_addr_back / 2;
    while (!found)
    {
        unit_read(flash, unit, tmp_addr, &tmp_data, 2 * sizeof(uint8_t));
        if (tmp_data[0] == 0xff && tmp_data[1] == 0xff)
        { // 向左找
            tmp_addr_back = tmp_addr;
            tmp_addr = (tmp_addr_back - tmp_addr_front) / 2;
        }
        else if (tmp_data[0] != 0xff && tmp_data[1] != 0xff)
        { // 向右找
            tmp_addr_front = tmp_addr;
            tmp_addr = (tmp_addr_front + tmp_addr_back) / 2;
        }
        else
        {
            found = true;
            return tmp_addr + 1;
        }
    }
}

bool flash_init(tFlash *flash, FlashHandle handle,
                tFlashDriverBaseOps *ops, tFlashDriverIAPOps *iap_ops)
{
    if (!flash || !handle || !ops)
        return false;
    flash->handle = handle;
    flash->base_ops = ops;
    flash->iap_ops = iap_ops;
    flash->base_ops->Init(&flash->handle, &flash->usr_sector_id, &flash->num_usr_sectors);

    flash->status_register_sectors = 0xff;
    flash->usr_flash_size = 0;
    for (uint8_t i = 0; i < flash->num_usr_sectors; i++)
    {
        flash->status_register_sectors &= (0xfe << i); // 标记未注册扇区
        flash->usr_flash_size += flash->base_ops->Get_sector_size(handle, flash->usr_sector_id[i]);
    }

    flash->usr_sectors_used = 0;
    flash->usr_sectors_free = flash->num_usr_sectors;

    return true;
}

tFlashUnit *flash_register_unit(tFlash *flash)
{
    if (!flash)
        return NULL;

    // 如果 没有剩余可注册扇区，则返回失败
    if (0 >= flash->usr_sectors_free)
        return NULL;

    // 有可以注册的扇区
    tFlashUnit *new_unit = (tFlashUnit *)malloc(sizeof(tFlashUnit));
    if (NULL == new_unit)
        return NULL; // 内存分配失败

    // 自动分配空闲扇区 从最低扇区开始
    for (eFlashSector sec = FLASH_SECTOR_0; sec < flash->num_usr_sectors; sec++)
    {
        if (flash->status_register_sectors & (1 << sec))
        {
            // 扇区已注册，跳过
            continue;
        }

        // 找到空闲扇区号，注册它
        new_unit->sector = sec;
        new_unit->sector_start_addr = flash->base_ops->Get_sector_addr(flash->handle, sec);
        if (NULL != new_unit->sector_start_addr)
        {
            new_unit->size = flash->base_ops->Get_sector_size(flash->handle, sec); // 获取扇区大小

            flash->status_register_sectors |= (1 << sec); // 标记为已注册

            flash->usr_sectors_used++;
            flash->usr_sectors_free--;

            new_unit->free_addr = unit_get_free_addr(flash, new_unit);  // 获取空闲空间起始地址
            new_unit->free_size = new_unit->size - new_unit->free_addr; // 计算空闲空间大小
            return new_unit;                                            // 返回新注册的扇区
        }
        else
        { // 注册扇区失败，释放内存
            free(new_unit);
            new_unit = NULL;
            return NULL;
        }
    }

    return NULL; // 没有剩余可注册扇区，返回失败
}
void flash_free_unit(tFlash *flash, tFlashUnit *unit)
{
    if (!flash || !unit)
        return;

    // 更新状态
    flash->status_register_sectors &= ~(1 << unit->sector);              // 标记为未注册
    flash->base_ops->Erase_sector(flash->handle, (uint8_t)unit->sector); // 擦除扇区
    flash->usr_sectors_used--;
    flash->usr_sectors_free++;

    free(unit);
    unit = NULL;
}
bool flash_unit_write(tFlash *flash, tFlashUnit *unit, const uint8_t *data, uint16_t size)
{
    if (!flash || !unit || !data)
        return false;
    if (size > unit->free_size)
    {
        // 先擦除
        if (!unit_erase(flash, unit))
            return false;
    }

    return unit_write(flash, unit, unit->free_addr, data, size); // 写入数据
}
bool flash_unit_read(tFlash *flash, tFlashUnit *unit, uint8_t *data, uint16_t size)
{
    if (!flash || !unit || !data)
        return false;
    if (unit->free_addr < size)
    {
        data = NULL; // 数据为空 表示之前还未有数据写入或者刚擦除
        return true;
    }
    unit_read(flash, unit, unit->free_addr - size, data, size); // 读取该类型数据
    return true;
}
bool flash_unit_erase(tFlash *flash, tFlashUnit *unit)
{
    if (!flash || !unit)
        return false;
    if (!unit_erase(flash, unit))
        return false;
    return true;
}

uint8_t flash_get_status(tFlash *flash)
{
    if (!flash)
        return 0;
    return flash->base_ops->Get_status(&flash->handle); // 获取状态
}

// bl app 相关函数
tFlashIAP *flash_register_iap(tFlash *flash)
{
    if (!flash)
        return NULL;
    tFlashIAP *new_iap = (tFlashIAP *)malloc(sizeof(tFlashIAP));
    if (NULL == new_iap)
        return NULL; // 内存分配失败
    if (NULL == flash->iap_ops)
        return NULL;
    flash->iap_ops->Get_IAP_info(flash->handle, new_iap);
    return new_iap;
}

void flash_free_iap(tFlash *flash, tFlashIAP *iap)
{
    if (!flash || !iap)
        return;
    free(iap);
    iap = NULL;
}

bool flash_erase_app(tFlash *flash, tFlashIAP *iap)
{
    if (!flash)
        return false;
    for (uint8_t i = 0; i < iap->num_app_sectors; i++)
    {
        if (!flash->base_ops->Erase_sector(flash->handle, (uint8_t)iap->app_sector_id[i]))
            return false;
    }
    return true;
}
bool flash_write_app(tFlash *flash, tFlashIAP *iap,
                     uint32_t addr, const uint8_t *data, uint16_t size)
{
    if (!flash || !iap)
        return false;
    return flash->base_ops->Write(flash->handle, data, addr + iap->app_addr, size);
}
bool flash_verify_app(tFlash *flash, tFlashIAP *iap,
                      uint32_t addr, const uint8_t *data, uint16_t size)
{
    if (!flash || !iap)
        return false;
    uint8_t *verify_data = malloc(size * sizeof(uint8_t));
    flash->base_ops->Read(flash->handle, addr + iap->app_addr, verify_data, size);
    for (uint16_t i = 0; i < size; i++)
    {
        if (verify_data[i] != data[i])
        {
            free(verify_data);  // 释放内存
            verify_data = NULL; // 释放内存
            return false;
        }
    }
    free(verify_data);  // 释放内存
    verify_data = NULL; // 释放内存
    return true;
}

bool flash_erase_bl(tFlash *flash, tFlashIAP *iap)
{
    if (!flash)
        return false;
    for (uint8_t i = 0; i < iap->num_bl_sectors; i++)
    {
        if (!flash->base_ops->Erase_sector(flash->handle, (uint8_t)iap->bl_sector_id[i]))
            return false;
    }
    return true;
}
bool flash_write_bl(tFlash *flash, tFlashIAP *iap,
                    uint32_t addr, const uint8_t *data, uint16_t size)
{
    if (!flash || !iap)
        return false;
    return flash->base_ops->Write(flash->handle, data, addr + iap->bl_addr, size);
}
bool flash_verify_bl(tFlash *flash, tFlashIAP *iap,
                     uint32_t addr, const uint8_t *data, uint16_t size)
{
    if (!flash || !iap)
        return false;
    uint8_t *verify_data = malloc(size * sizeof(uint8_t));
    flash->base_ops->Read(flash->handle, addr + iap->bl_addr, verify_data, size);
    for (uint16_t i = 0; i < size; i++)
    {
        if (verify_data[i] != data[i])
        {
            free(verify_data);  // 释放内存
            verify_data = NULL; // 释放内存
            return false;
        }
    }
    free(verify_data);  // 释放内存
    verify_data = NULL; // 释放内存
    return true;
}

bool flash_jump_to_app(tFlash *flash, tFlashIAP *iap)
{
    if (!flash || !iap)
        return false;

    return flash->iap_ops->Jump_to_addr(flash->handle, iap->app_addr);
}
bool flash_jump_to_bl(tFlash *flash)
{
    if (!flash)
        return false;
    flash->iap_ops->System_reset(flash->handle);
    return true;
}
