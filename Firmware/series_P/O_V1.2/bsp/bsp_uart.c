#include "bsp_uart.h"
#include "usart.h"
#include "config.h"

// ============================================
// UART - 串口
// ============================================
void bsp_uart_receive_dma(uint8_t *data, u16 len)
{
    HAL_UART_Receive_DMA(&UART_CH, data, len);
}
bool bsp_uart_transmit_dma(uint8_t *data, u16 len)
{
    return HAL_UART_Transmit_DMA(&UART_CH, data, len) == HAL_OK;
}

__weak void bsp_uart_rx_callback()
{
    return;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART_INSTANCE)
    {
        bsp_uart_rx_callback();
    }
}
