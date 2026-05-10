#ifndef __DEVICE_H
#define __DEVICE_H

#include "bsp_spi.h"
#include "bsp_led.h"
#include "protocol_defs.h"
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

/* 芯片特性描述 */
typedef struct
{
    uint16_t resolution;     /* 单圈分辨率，如 16384 */
    float deg_per_lsb;       /* 每 LSB 角度 */
    uint16_t cmd_high;       /* 读高位命令 (MT6816 专用) */
    uint16_t cmd_low;        /* 读低位命令 (MT6816 专用) */
    uint16_t cmd_read_angle; /* 单次读角度命令 (如 AS5047) */
    u8 spi_CPOL;             /* SPI 时钟极性 */
    u8 spi_CPHA;             /* SPI 时钟相位 */
    u8 spi_data_size;        /* SPI 数据宽度 */
    bool (*parse_and_check)(uint16_t raw_high, uint16_t raw_low, uint16_t *angle_out);
} tEncoderChipDesc;

/* 编码器全局实例 */
typedef struct
{
    volatile uint8_t state;        /* 状态机当前状态 */
    volatile uint16_t no_resp_tic; /* 超时计数器 */

    volatile uint16_t cmd_reg;       /* 待发送命令 */
    volatile uint16_t shadow_raw[2]; /* DMA 接收缓冲 */
    volatile uint16_t data_raw[2];   /* 处理用数据副本 */

    const tEncoderChipDesc *chip_desc; /* 当前芯片描述 */
    eEncoderChip chip_type;            /* 当前芯片型号 */

    uint16_t angle_raw;
    uint16_t angle_raw_last;
    uint16_t pos_offset;
    float angle_abs;
    float pos;
    float pos_last;
    float omega_rpm;
    uint32_t last_time;
    int32_t num_turns;

    bool first_run;
    uint8_t valid;
    uint8_t rubbish_data_tic;
} tEncoderInstance;
// 全局唯一实例
extern tEncoderInstance g_encoder;

bool fEncoder_Init(eEncoderChip type);
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