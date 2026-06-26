#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "bsp_base.h"

// ============================================
// UART - 串口
// ============================================
void bsp_uart_receive_dma(u8 *data, u16 len);
bool bsp_uart_transmit_dma(u8 *data, u16 len);
void bsp_uart_rx_callback(void);

#endif // __BSP_UART_H