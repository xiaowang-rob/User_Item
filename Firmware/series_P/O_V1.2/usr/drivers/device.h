#ifndef __DEVICE_H
#define __DEVICE_H

#include "bsp_spi.h"
#include "bsp_led.h"
#include "protocol.h"
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
    bool use_dma_state_machine;         // 是否使用DMA状态机
    void (*dma_state_entry)(void *enc); // DMA状态机入口（按芯片不同）
    u8 dma_post_high_state;             // WAIT_HIGH 完成后的下一个状态
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
    int32_t num_turns;

    bool first_run;
    uint8_t valid;
    uint8_t rubbish_data_tic;

    /* PLL 角度/速度联合估计 */
    float pll_theta_delta; /* PLL 误差 [deg] */
    float pll_theta;       /* PLL 输出角度 [deg] */
    float pll_omega_rpm;   /* PLL 输出速度 [rpm] */
    float pll_kp;          /* PLL 比例增益 */
    float pll_ki;          /* PLL 积分增益 */
    float pll_integ;       /* PLL 积分累加 */

    /* SPI 通信错误率 (指数移动平均) */
    float spi_error_rate;
} tEncoderInstance;

bool encoder_init(eEncoderChip type);
void encode_clear_error_flag(void);
void encoder_main_loop_task(void);
void encoder_pll_update(float dt);
float encoder_get_angle_abs(void);
float encoder_get_angle_inc(void);
float encoder_get_rpm(void);
int encoder_get_num_turns(void);

void encoder_set_angle_zero(void);

// flash相关定义

/* 存储器规格定义 ---------------------------------------------------------*/
#define FLASH_SIZE (128 / 8 * 1024 * 1024) ///< 总容量：16MB
#define FLASH_PAGE_SIZE 256                ///< 页大小：256字节
#define FLASH_PAGE_NUM 16                  ///< 页数量：16页
#define FLASH_SECTOR_NUM 16                ///< 扇区数量：16个
#define FLASH_BLOCK_NUM 256                ///< 块数量：256块

/* 函数声明 -------------------------------------------------------------*/
void flash_init(void);
void flash_erase_one_sector(u32 Address);
void flash_erase_sector(u32 Address, u32 Write_data_NUM);
bool flash_read_data(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool flash_write_word(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);

void led_control(eDeviceStatus can_state, eDeviceStatus encoder_state);

#endif