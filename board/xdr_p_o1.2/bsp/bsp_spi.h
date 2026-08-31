#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "bsp_base.h"

// ============================================
// SPI - 编码器通信
// ============================================

void bsp_encoder_register_callback(void (*callback)(void *), void *user_arg);

// 底层配置默认为 CPOL=1, CPHA=1, datasize=16bit
bool bsp_change_encoder_spi_config(u8 CPOL, u8 CPHA, u8 datasize);
void bsp_int_encoder_cs(bool active);
void bsp_ext_encoder_cs(bool active);
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