#ifndef __FLASH_ABS_H
#define __FLASH_ABS_H

#include <stdint.h>
#include <stdbool.h>

// 分配扇区号 - 8个 用byte存储状态
typedef enum
{
    FLASH_SECTOR_0 = 0,
    FLASH_SECTOR_1,
    FLASH_SECTOR_2,
    FLASH_SECTOR_3,
    FLASH_SECTOR_4,
    FLASH_SECTOR_5,
    FLASH_SECTOR_6,
    FLASH_SECTOR_7
} eFlashSector;

// 单个注册存储单元管理 - 因为擦除以扇区为单元 - 所以这里规定 一个扇区为一个单元 一个单元只能负责一种功能
// 比如 注册单元0 存储参数信息 到时候存和读都只在这个扇区操作 不涉及其他的数据
// 只会在单元满时擦除 增长flash使用受命
typedef struct
{
    eFlashSector sector;        // 当前扇区号
    uint32_t sector_start_addr; // 当前绑定扇区起始地址
    uint32_t free_addr;         // 标记地址
    uint32_t size;              // 当前单元大小
    uint32_t free_size;         // 当前单元剩余空间
} tFlashUnit;

typedef struct
{
    uint32_t bl_addr;  // Bootloader区起始地址
    uint32_t app_addr; // 应用区起始地址

    uint8_t num_bl_sectors;       // bl 扇区数
    uint8_t num_app_sectors;      // app 扇区数
    const uint8_t *bl_sector_id;  // bl 扇区号
    const uint8_t *app_sector_id; // app 扇区号
} tFlashIAP;

// flash 驱动 句柄
typedef void *FlashHandle;

// flash 驱动 函数表
typedef struct
{
    // 基本接口
    bool (*Init)(FlashHandle *handle, uint8_t *usr_sector_id, uint8_t *num_usr_sectors);     // 初始化flash硬件
    uint32_t (*Get_sector_size)(FlashHandle handle, uint8_t sec_idx);                        // 注册扇区
    uint32_t (*Get_sector_addr)(FlashHandle handle, uint8_t sec_idx);                        // 注册扇区
    bool (*Erase_sector)(FlashHandle handle, uint8_t sec_idx);                               // 擦除扇区
    bool (*Erase_chip)(FlashHandle handle);                                                  // 擦除整个芯片-一般不用
    bool (*Read)(FlashHandle handle, uint32_t address, uint8_t *data, uint32_t size);        // 读取数据
    bool (*Write)(FlashHandle handle, uint32_t address, const uint8_t *data, uint32_t size); // 写入数据
    uint8_t (*Get_status)(FlashHandle handle);                                               // 获取flash状态
} tFlashDriverBaseOps;

typedef struct
{
    void (*Get_IAP_info)(FlashHandle handle, tFlashIAP *iap); // 获取IAP信息
    bool (*Jump_to_addr)(FlashHandle handle, uint32_t addr);  // 跳转到应用区
    void (*System_reset)(FlashHandle handle)
} tFlashDriverIAPOps;

// flash 对象
typedef struct
{
    tFlashDriverBaseOps *base_ops;
    tFlashDriverIAPOps *iap_ops;
    FlashHandle handle;

    uint8_t num_usr_sectors;      // usr 扇区数
    const uint8_t *usr_sector_id; // usr 扇区号

    uint16_t status_register_sectors; // 扇区注册状态 0-还未注册 1-已注册/不能使用

    uint8_t usr_sectors_used; // 已使用扇区数
    uint8_t usr_sectors_free; // 剩余扇区数
    uint32_t usr_flash_size;  // 总容量

} tFlash;

// flash 存储单元接口
bool flash_init(tFlash *flash, FlashHandle handle,
                tFlashDriverBaseOps *ops, tFlashDriverIAPOps *iap_ops);
tFlashUnit *flash_register_unit(tFlash *flash);
void flash_free_unit(tFlash *flash, tFlashUnit *unit);
bool flash_unit_write(tFlash *flash, tFlashUnit *unit, const uint8_t *data, uint16_t size);
bool flash_unit_read(tFlash *flash, tFlashUnit *unit, uint8_t *data, uint16_t size);
bool flash_unit_erase(tFlash *flash, tFlashUnit *unit);
uint8_t flash_get_status(tFlash *flash);

// mcu 特殊接口 用于 Bootloader更新  和 固件升级
tFlashIAP *flash_register_iap(tFlash *flash);

void flash_free_iap(tFlash *flash, tFlashIAP *iap);

bool flash_erase_app(tFlash *flash, tFlashIAP *iap);
bool flash_write_app(tFlash *flash, tFlashIAP *iap,
                     uint32_t addr, const uint8_t *data, uint16_t size);
bool flash_verify_app(tFlash *flash, tFlashIAP *iap,
                      uint32_t addr, const uint8_t *data, uint16_t size);
bool flash_erase_bl(tFlash *flash, tFlashIAP *iap);
bool flash_write_bl(tFlash *flash, tFlashIAP *iap,
                    uint32_t addr, const uint8_t *data, uint16_t size);
bool flash_verify_bl(tFlash *flash, tFlashIAP *iap,
                     uint32_t addr, const uint8_t *data, uint16_t size);

bool flash_jump_to_app(tFlash *flash, tFlashIAP *iap);
bool flash_jump_to_bl(tFlash *flash);

#endif // __FLASH_ABS_H