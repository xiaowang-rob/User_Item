#ifndef __DEVICE_H
#define __DEVICE_H

#include "bsp_spi.h"
#include "bsp_led.h"

// 外设状态
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
typedef enum
{
    ENCODER_STATE_START_READ,
    ENCODER_STATE_WAIT_HIGH,
    ENCODER_STATE_WAIT_LOW,
    ENCODER_STATE_PROCESS_DATA
} eEncoderState_DMA;

typedef struct
{
    eEncoderState_DMA state;
    float angle_abs;
    float omega_rpm;
    float pos;
    u16 angle_raw;
    u16 angle_raw_last;
    u16 pos_offset;

    u32 last_time;
    int num_turns;
    int num_turns_last;
} tEncoder;

#define MT6816_REG_ANGLE_HIGH 0x03
#define MT6816_REG_ANGLE_LOW 0x04
#define MT6816_REG_STATUS 0x05
#define MT6816_NO_MAG_WARNING (1 << 1)
#define MT6816_PARITY_CHECK (1 << 0)

void fEncoderMainLoopTask(void);
float fGetEncoderAngle_ABS(void);
float fGetEncoderAngle_INC(void);
float fGetEncoderRPM(void);
int fGetEncoderNumTurns(void);

void fSetEncoderAngleZero(void);

// flash相关定义

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

void fLED_Control(eDeviceStatus can_state, eDeviceStatus encoder_state);
#endif