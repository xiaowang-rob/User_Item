#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "bsp.h"

/* ============================================
 * UART - 串口
 * ============================================ */
void BSP_UART_Receive_DMA(u8 *data, u16 len);
bool BSP_UART_Transmit_DMA(u8 *data, u16 len);
void BSP_UART_RxCallback(void);

#endif /* __BSP_UART_H */