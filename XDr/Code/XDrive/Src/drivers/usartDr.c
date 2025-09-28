#include "usartDr.h"
#include "usart.h"
#include "string.h"

Usart_Farme_t UsartRxFrame_g = {0};
Usart_Farme_t UsartTxFrame_g = {0};
u8 _rx;
u8 _tx;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == &huart1)
    {
        usartRecvByte(&_rx);
    }
}
void usartDrInit()
{
    memset(&UsartRxFrame_g, 0, sizeof(Usart_Farme_t));
    memset(&UsartTxFrame_g, 0, sizeof(Usart_Farme_t));
    UsartTxFrame_g.msgID = Data;
    UsartTxFrame_g.head = PACKET_HEAD;
    UsartTxFrame_g.tail = PACKET_TAIL;

    UsartRxFrame_g.msgID = Data;
    UsartRxFrame_g.head = PACKET_HEAD;
    UsartRxFrame_g.tail = PACKET_TAIL;
    HAL_UART_Receive_DMA(&huart1, _rx, 1);
}
void usartSendByte(u8 *data)
{
    HAL_UART_Transmit_DMA(&huart1, data, 1);
}
void usartSendData(u8 *data, u8 len)
{
    HAL_UART_Transmit_DMA(&huart1, data, len);
}
void usart_frame_send(msgID_e id, u8 *data, u8 len)
{
    UsartTxFrame_g.msgID = id;
    UsartTxFrame_g.len = len;
    memcpy(UsartTxFrame_g.data, data, len);
    usartSendByte(&UsartTxFrame_g.head);
    usartSendByte(&UsartTxFrame_g.msgID);
    usartSendByte(&UsartTxFrame_g.len);
    usartSendData(UsartTxFrame_g.data, len);
    usartSendByte(&UsartTxFrame_g.tail);
}
void read_usartrx_data(u8 *id, u8 *data, u8 *len)
{
    *id = UsartRxFrame_g.msgID;
    *len = UsartRxFrame_g.len;
    memcpy(data, UsartRxFrame_g.data, UsartRxFrame_g.len);
}
bool first_flag = true;
void usartRecvByte(u8 *data)
{
    if (*data == UsartTxFrame_g.head)
    { // 得到包头
        if (*data == UsartRxFrame_g.tail)
        { // 接收到尾部'
            UsartRxFrame_g.Index = 0;
            UsartRxFrame_g.len = UsartRxFrame_g.Index;
            HAL_UART_Receive_DMA(&huart1, data, 1);
        }
        if (first_flag)
            UsartRxFrame_g.msgID = *data;
        UsartRxFrame_g.data[++UsartRxFrame_g.Index] = *data;
    }
    HAL_UART_Receive_DMA(&huart1, data, 1);
}

void vofa_send_float(float value)
{
    u8 buffer[4];

    // 将 float 转换为字节数组（小端序）
    memcpy(buffer, &value, 4);
    usartSendData(buffer, 4);
}
void vofa_send_multi_float(const float *data, u8 count)
{
    if (count == 0 || count > 10)
        return; // 限制最大数量

    u8 buffer[40]; // 最多 10 个 float * 4 字节
    u8 total_bytes = count * 4;

    // 将多个 float 复制到缓冲区
    for (u8 i = 0; i < count; i++)
    {
        memcpy(&buffer[i * 4], &data[i], 4);
        // 发送所有数据
        usartSendData(buffer, total_bytes);
    }
}