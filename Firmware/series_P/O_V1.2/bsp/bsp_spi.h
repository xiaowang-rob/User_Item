#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "bsp.h"

void BSP_Encoder_SPI_TxRxCpltCallback(void); // SPI传输完成回调函数声明
void BSP_Encoder_SPI_ErrorCallback(void);    // SPI错误回调函数声明

/* ============================================
 * SPI - 编码器通信
 * ============================================ */
typedef enum
{
    INTERNAL,
    EXTERNAL
} eEncoderType;

bool BSP_SetEncoder_SPI_Config(u8 CPOL, u8 CPHA, u8 datasize);
void BSP_Encoder_CS(eEncoderType type, bool level);
bool BSP_Encoder_SPI_IS_READY();
bool BSP_Encoder_SPI_TransmitReceive_DMA(u8 *tx, u8 *rx, u16 len);
void BSP_Encoder_SPI_Abort();
void BSP_Encoder_SPI_CLEAR_DMA_error_flags();

/* ============================================
 * SPI - flash通信
 * ============================================ */
void BSP_Flash_CS(bool level);
bool BSP_Flash_SPI_Transmit(u8 *tx, u16 len, u32 timeout);
bool BSP_Flash_SPI_Receive(u8 *rx, u16 len, u32 timeout);

#endif /* __BSP_SPI_H */