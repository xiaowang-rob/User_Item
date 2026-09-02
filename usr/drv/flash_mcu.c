#include "device.h"
#include "bsp_flash.h"

// 这一层flash驱动访问的都是MCU flash 的真实扇区和地址 传入也是绝对地址

// MCU flash 配置

// 驱动上下文
typedef struct
{
    eDeviceStatus Dstate; // 设备状态

} tMcuFlash_ctx;

// 函数操作表

static bool mcu_flash_init(FlashHandle *handle, uint8_t *usr_sector_id, uint8_t *num_usr_sectors);
static uint32_t mcu_flash_get_sector_size(FlashHandle handle, uint8_t sec_idx);
static uint32_t mcu_flash_get_sector_addr(FlashHandle handle, uint8_t sec_idx); // 注册扇区
static bool mcu_flash_erase_sector(FlashHandle handle, uint8_t sec_idx);
static bool mcu_flash_read_data(FlashHandle handle, uint32_t address, uint8_t *data, uint32_t size);
static bool mcu_flash_write_data(FlashHandle handle, uint32_t address, const uint8_t *data, uint32_t size);

static uint8_t mcu_flash_get_status(FlashHandle handle);

static void mcu_flash_get_iap_info(FlashHandle handle, tFlashIAP *iap);
static bool mcu_flash_jump_to_addr(FlashHandle handle, uint32_t addr);
static void mcu_system_reset(FlashHandle handle);

tFlashDriverBaseOps mcu_flash_driver_baseops = {
    .Init = mcu_flash_init,
    .Get_sector_size = mcu_flash_get_sector_size,
    .Get_sector_addr = mcu_flash_get_sector_addr, // 注册扇区
    .Erase_sector = mcu_flash_erase_sector,
    .Erase_chip = NULL, // mcu 不擦除整个芯片
    .Read = mcu_flash_read_data,
    .Write = mcu_flash_write_data,
};

tFlashDriverIAPOps mcu_flash_driver_iapops = {
    .Get_IAP_info = mcu_flash_get_iap_info,
    .Jump_to_addr = mcu_flash_jump_to_addr,
    .System_reset = mcu_system_reset,
};

FlashHandle mcu_flash_create(void)
{
    tMcuFlash_ctx *ctx = (tMcuFlash_ctx *)calloc(1, sizeof(tMcuFlash_ctx));
    if (!ctx)
        return NULL;
    return ctx;
}
void mcu_flash_destroy(FlashHandle handle)
{
    if (!handle)
        return;
    free(handle);
    handle = NULL;
}

static bool mcu_flash_init(FlashHandle *handle,
                           uint8_t *usr_sector_id, uint8_t *num_usr_sectors)
{
    if (!handle)
        return false;
    tMcuFlash_ctx *ctx = (tMcuFlash_ctx *)handle;
    // 这里一般初始化系统flash 但是HAL库已经初始化了
    usr_sector_id = bsp_flash_get_usr_config(num_usr_sectors);
    ctx->Dstate = ONLINE;
    return true;
}
static uint32_t mcu_flash_get_sector_size(FlashHandle handle, uint8_t sec_idx)
{
    if (!handle)
        return 0;
    return bsp_flash_get_sector_size(sec_idx);
}
static uint32_t mcu_flash_get_sector_addr(FlashHandle handle, uint8_t sec_idx)
{
    if (!handle)
        return NULL;
    return bsp_flash_get_sector_start_addr(sec_idx);
}
static bool mcu_flash_erase_sector(FlashHandle handle, uint8_t sec_idx)
{
    if (!handle)
        return false;
    return bsp_flash_erase_sector(sec_idx);
}

static bool mcu_flash_read_data(FlashHandle handle,
                                uint32_t address, uint8_t *data, uint32_t size)
{
    if (!handle)
        return false;
    return bsp_flash_read_data(address, data, size);
}
static bool mcu_flash_write_data(FlashHandle handle,
                                 uint32_t address, const uint8_t *data, uint32_t size)
{
    if (!handle)
        return false;
    return bsp_flash_write_word(address, data, size);
}
static uint8_t mcu_flash_get_status(FlashHandle handle)
{
    if (!handle)
        return 0;
    tMcuFlash_ctx *ctx = (tMcuFlash_ctx *)handle;
    return ctx->Dstate;
}

static void mcu_flash_get_iap_info(FlashHandle handle, tFlashIAP *iap)
{
    iap->bl_sector_id = bsp_flash_get_bl_config(&iap->num_bl_sectors);
    iap->app_sector_id = bsp_flash_get_app_config(&iap->num_app_sectors);
    iap->bl_addr = bsp_flash_get_sector_start_addr(iap->bl_sector_id[0]);
    iap->app_addr = bsp_flash_get_sector_start_addr(iap->app_sector_id[0]);
}
static bool mcu_flash_jump_to_addr(FlashHandle handle, uint32_t addr)
{
    if (!handle)
        return false;
    return bsp_jump_addr(addr);
}
static void mcu_system_reset(FlashHandle handle)
{
    if (!handle)
        return;
    bsp_system_reset();
}
