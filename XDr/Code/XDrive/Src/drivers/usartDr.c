#include "usartDr.h"
#include "usart.h"
#include "string.h"
#include "stdbool.h"
Usart_Farme_t UsartRxFrame_g = {0};
Usart_Farme_t UsartTxFrame_g = {0};
u8 _rx;
u8 _tx;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        usartRecvByte(&_rx);
    }
}
void usartDrInit()
{
    memset(&UsartRxFrame_g, 0, sizeof(Usart_Farme_t));
    memset(&UsartTxFrame_g, 0, sizeof(Usart_Farme_t));
    UsartTxFrame_g.head = PACKET_HEAD;
    UsartTxFrame_g.tail = PACKET_TAIL;

    UsartRxFrame_g.head = PACKET_HEAD;
    UsartRxFrame_g.tail = PACKET_TAIL;
    HAL_UART_Receive_DMA(&huart1, &_rx, 1);
}
void usartSendByte(u8 *data)
{
    HAL_UART_Transmit_DMA(&huart1, data, 1);
}
void usartSendData(u8 *data, u8 len)
{
    HAL_UART_Transmit_DMA(&huart1, data, len);
}
void usart_frame_send(USART_MSG_ID_e id, u8 *data, u8 len)
{
    if (len == 0)
        return;
    UsartTxFrame_g.msgID = id;
    UsartTxFrame_g.len = len;
    u16 temp;
    for (int i = 0; i < len; i++)
        temp += data[i];
    UsartTxFrame_g.check = (u8)temp & 0x01;
    memcpy(UsartTxFrame_g.data, data, len);
    usartSendByte(&UsartTxFrame_g.head);
    usartSendByte(&UsartTxFrame_g.msgID);
    usartSendByte(&UsartTxFrame_g.len);
    usartSendData(UsartTxFrame_g.data, len);
    usartSendByte(&UsartTxFrame_g.check);
    usartSendByte(&UsartTxFrame_g.tail);
}
// 接收到数据帧的处理函数
__weak void usart_farmedata_deal(u8 id, u8 *data, u8 len)
{
    return;
}
static bool get_head = false;
static u16 check = 0;
static u8 DataIndex = 0;
void usartRecvByte(u8 *data)
{
    if (get_head)
    {
        if (*data == UsartRxFrame_g.tail)
        {
            if (check == UsartRxFrame_g.check)
                usart_farmedata_deal(UsartRxFrame_g.msgID, UsartRxFrame_g.data, UsartRxFrame_g.len);
            get_head = false;
        }
        else
        {
            if (DataIndex == 0)
                UsartRxFrame_g.msgID = *data;
            if (DataIndex == 1)
                UsartRxFrame_g.len = *data;
            if (DataIndex >= 2 && DataIndex < 2 + UsartRxFrame_g.len)
            {
                UsartRxFrame_g.data[DataIndex - 2] = *data;
                check += *data;
            }
            if (DataIndex == 2 + UsartRxFrame_g.len)
            {
                UsartRxFrame_g.check = *data;
                check = check && 0x01;
            }
            if (DataIndex > 3 + UsartRxFrame_g.len)
                get_head = false;
            DataIndex++;
        }
    }
    else
    {
        if (*data == UsartTxFrame_g.head)
        {
            get_head = true;
            DataIndex = 0;
            check = 0;
        }
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
    if (count == 0 || count > 8)
        return; // 限制最大数量

    u8 buffer[32]; // 最多 8 个 float * 4 字节
    u8 total_bytes = count * 4;

    // 将多个 float 复制到缓冲区
    for (u8 i = 0; i < count; i++)
    {
        memcpy(&buffer[i * 4], &data[i], 4);
        // 发送所有数据
        usartSendData(buffer, total_bytes);
    }
}