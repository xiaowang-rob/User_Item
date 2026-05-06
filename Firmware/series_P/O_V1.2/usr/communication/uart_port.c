/**
 * @file uart_port.c
 * @brief UART通信端口驱动实现
 * @details 实现UART协议帧收发、数据解析及VOFA+上位机数据传输功能
 */

#include "uart_port.h"
#include "string.h"

/* 全局收发帧结构体 */
tUartFrame UsartRxFrame_g = {0}; ///< 接收帧缓冲区
tUartFrame UsartTxFrame_g = {0}; ///< 发送帧缓冲区

/* DMA收发临时缓冲区 */
u8 _rx;                            ///< DMA接收单字节缓冲
u8 _tx;                            ///< DMA发送单字节缓冲（未使用）
u8 _tx_data[MAX_FRAME_LENGTH + 5]; ///< 帧组装缓冲区（含头尾校验）

/**
 * @brief UART端口初始化
 * @note 1. 初始化收发帧结构体，设置协议帧头尾
 *       2. 启动DMA循环接收（每次接收1字节）
 */
void fUartPortInit(void)
{
    memset(&UsartRxFrame_g, 0, sizeof(tUartFrame));
    memset(&UsartTxFrame_g, 0, sizeof(tUartFrame));

    /* 设置协议帧头尾标识 */
    UsartTxFrame_g.head = PACKET_HEAD;
    UsartTxFrame_g.tail = PACKET_TAIL;
    UsartRxFrame_g.head = PACKET_HEAD;
    UsartRxFrame_g.tail = PACKET_TAIL;

    /* 启动DMA单字节循环接收 */
    BSP_UART_Receive_DMA(&_rx, 1);
}

/**
 * @brief UART单字节发送
 * @param data 待发送字节指针
 */
void uartSendByte(u8 *data)
{
    BSP_UART_Transmit_DMA(data, 1);
}

/**
 * @brief UART多字节发送
 * @param data 待发送数据缓冲区指针
 * @param len  发送数据长度
 */
void uartSendData(u8 *data, u8 len)
{
    BSP_UART_Transmit_DMA(data, len);
}

/**
 * @brief 发送协议数据帧
 * @param id   消息ID
 * @param data 待发送数据指针
 * @param len  数据长度（1~MAX_FRAME_LENGTH）
 * @note 帧格式：头(1)+ID(1)+长度(1)+数据(N)+校验(1)+尾(1)
 *       校验算法：数据字节累加后取最低位
 */
void fUartPortSendFrame(u8 id, u8 *data, u8 len)
{
    if (len == 0)
        return;

    /* 填充帧头信息 */
    UsartTxFrame_g.msgID = id;
    UsartTxFrame_g.len = len;

    /* 计算校验值（数据累加取最低位） */
    u16 temp = 0;
    for (int i = 0; i < len; i++)
        temp += data[i];
    UsartTxFrame_g.check = (u8)temp & 0xff;

    /* 复制数据到帧结构 */
    memcpy(UsartTxFrame_g.data, data, len);

    /* 组装完整数据帧 */
    _tx_data[0] = UsartTxFrame_g.head;              // 帧头
    _tx_data[1] = UsartTxFrame_g.msgID;             // 消息ID
    _tx_data[2] = UsartTxFrame_g.len;               // 数据长度
    memcpy(_tx_data + 3, UsartTxFrame_g.data, len); // 数据域
    _tx_data[3 + len] = UsartTxFrame_g.check;       // 校验字节
    _tx_data[4 + len] = UsartTxFrame_g.tail;        // 帧尾

    /* 启动DMA发送完整帧 */
    uartSendData(_tx_data, 5 + len);
}

/**
 * @brief 接收帧数据处理回调（弱定义）
 * @param id   消息ID
 * @param data 接收数据缓冲区
 * @param len  有效数据长度
 * @note 用户可在应用层重写此函数实现业务逻辑
 */
__weak void fUartRxFrameCallback(u8 id, u8 *data, u8 len)
{
    return;
}

/* 接收状态机变量 */
static bool get_head = false; ///< 帧头检测标志
static u16 check = 0;         ///< 校验和累加器
static u8 DataIndex = 0;      ///< 当前解析字节索引

/**
 * @brief UART接收字节处理
 * @param data 指向接收字节的指针
 * @note 实现状态机协议解析：
 *       1. 检测帧头(PACKET_HEAD)进入接收状态
 *       2. 依次解析ID、长度、数据、校验、帧尾
 *       3. 校验通过后调用用户处理函数（调试模式跳过校验）
 *       4. 重新启动DMA接收下一字节
 */
void fUartReviceByte(u8 *data)
{
    if (get_head)
    {
        /* 检测帧尾 */
        if (*data == UsartRxFrame_g.tail)
        {
#ifndef __DEBUG__
            if (((u8)check & 0xff) == UsartRxFrame_g.check) // 校验通过
#endif
                fUartRxFrameCallback(UsartRxFrame_g.msgID, UsartRxFrame_g.data, UsartRxFrame_g.len);
            get_head = false; // 重置状态机
        }
        else
        {
            /* 按协议顺序解析各字段 */
            if (DataIndex == 0)
                UsartRxFrame_g.msgID = *data; // 消息ID
            else if (DataIndex == 1)
                UsartRxFrame_g.len = *data; // 数据长度
            else if (DataIndex >= 2 && DataIndex < 2 + UsartRxFrame_g.len)
            {
                UsartRxFrame_g.data[DataIndex - 2] = *data; // 数据域
                check += *data;                             // 累加校验和
            }
            else if (DataIndex == 2 + UsartRxFrame_g.len)
            {
                UsartRxFrame_g.check = *data; // 校验字节
            }
            else if (DataIndex > 3 + UsartRxFrame_g.len)
            {
                get_head = false; // 超长帧丢弃
            }
            DataIndex++;
        }
    }
    else if (*data == UsartTxFrame_g.head) // 检测帧头
    {
        /* 重置状态机 */
        get_head = true;
        DataIndex = 0;
        check = 0;
    }

    /* 重新启动DMA接收下一字节 */
    BSP_UART_Receive_DMA(data, 1);
}

/**
 * @brief UART接收完成中断回调
 * @param huart UART句柄指针
 * @note 仅处理USART1的接收中断，调用字节解析函数
 */
void BSP_UART_RxCallback()
{
    fUartReviceByte(&_rx);
}

/* VOFA+协议帧尾标识（固定4字节） */
static u8 tail_bytes[4] = {0x00, 0x00, 0x80, 0x7F};

/**
 * @brief VOFA+上位机浮点数据发送
 * @param data  浮点数数组指针
 * @param count 浮点数个数（1~8）
 * @note 数据格式：连续float数据 + 4字节帧尾(0x00 0x00 0x80 0x7F)
 *       适用于VOFA+上位机波形显示
 */
void fVOFA_FloatDataSend(const float *data, u8 count)
{
    if (count == 0 || count > 8)
        return;

    /* 复制浮点数据到发送缓冲区（小端格式） */
    for (u8 i = 0; i < count; i++)
    {
        memcpy(&_tx_data[i * 4], &data[i], 4);
    }

    /* 添加VOFA+协议帧尾 */
    memcpy(&_tx_data[count * 4], tail_bytes, 4);

    /* 启动DMA发送完整数据包 */
    uartSendData(_tx_data, count * 4 + 4);
}