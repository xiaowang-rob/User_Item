#ifndef __DEVICE_H
#define __DEVICE_H

#include "bsp_base.h"
#include "bsp_spi.h"
#include "bsp_led.h"
#include "protocol.h"

// 驱动状态
typedef enum
{
    OFFLINE,
    ONLINE,
    RUN_ERROR,
    RUNNING,
} eDeviceStatus;

typedef struct
{
    eDeviceStatus can_state;
    eDeviceStatus encoder_state;
    eDeviceStatus flash_state;
    eDeviceStatus usb_state;
} tDeviceStatus;

extern tDeviceStatus g_device_status;

// 编码器相关定义
EncoderChipHandle MT6816_create(void);
void MT6816_destroy(EncoderChipHandle handle);

extern tEncoderDriverOps MT6816_driver_ops;

EncoderChipHandle MT6835_create(void);
void MT6835_destroy(EncoderChipHandle handle);

extern tEncoderDriverOps MT6835_driver_ops;

EncoderChipHandle AS5047_create(void);
void AS5047_destroy(EncoderChipHandle handle);

extern tEncoderDriverOps AS5047_driver_ops;
// flash相关定义

// 存储器规格定义 ---------------------------------------------------------
#define FLASH_SIZE (128 / 8 * 1024 * 1024) ///< 总容量：16MB
#define FLASH_PAGE_SIZE 256                ///< 页大小：256字节
#define FLASH_PAGE_NUM 16                  ///< 页数量：16页
#define FLASH_SECTOR_NUM 16                ///< 扇区数量：16个
#define FLASH_BLOCK_NUM 256                ///< 块数量：256块

// 函数声明 -------------------------------------------------------------
void flash_init(void);
void flash_erase_one_sector(u32 Address);
void flash_erase_sector(u32 Address, u32 Write_data_NUM);
bool flash_read_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool flash_write_word(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);

void led_control(eDeviceStatus can_state, eDeviceStatus encoder_state);

#endif