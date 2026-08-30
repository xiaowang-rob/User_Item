#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "bsp_base.h"

void bsp_encoder_spi_txrx_cplt_callback(void); // SPI传输完成回调函数声明
void bsp_encoder_spi_error_callback(void);    // SPI错误回调函数声明

// ============================================
// SPI - 编码器通信
// ============================================
typedef enum
{
    INTERNAL,
    EXTERNAL
} eEncoderType;

// 兼容别名（与 eEncoderType 数值一致）
#define BSP_SPI_INTERNAL  INTERNAL   // 0
#define BSP_SPI_EXTERNAL  EXTERNAL   // 1

bool bsp_set_encoder_spi_config(u8 CPOL, u8 CPHA, u8 datasize);
void bsp_encoder_cs(eEncoderType type, bool level);
bool bsp_encoder_spi_is_ready();
bool bsp_encoder_spi_transmit_receive_dma(u8 *tx, u8 *rx, u16 len);
void bsp_encoder_spi_abort();
void bsp_encoder_spi_clear_dma_error_flags();

// ============================================
// SPI - flash通信
// ============================================
void bsp_flash_cs(bool level);
bool bsp_flash_spi_transmit(u8 *tx, u16 len, u32 timeout);
bool bsp_flash_spi_receive(u8 *rx, u16 len, u32 timeout);

#endif // __BSP_SPI_H