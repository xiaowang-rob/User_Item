/**
 * @file usb_port.c
 * @brief USB CDC通信端口驱动实现
 * @details 实现USB协议帧收发、数据封装及错误重发机制
 */

#include "usb_port.h"
#include "string.h"
#include "device.h"
/* USB协议帧全局变量 */
tUSB_Frame UsbTxFrame = {.head = USB_PACKET_HEAD, .tail = USB_PACKET_TAIL}; ///< 发送帧结构体
tUSB_Frame UsbRxFrame = {.head = USB_PACKET_HEAD, .tail = USB_PACKET_TAIL}; ///< 接收帧结构体

/* 传输错误计数器（用于重发机制） */
static u8 trans_fault_tic = 0;

// 上拉 让上位机识别到USB口
void fUSB_Init(void)
{
    BSP_USB_CS(true); // 使能USB时钟
}

/**
 * @brief USB数据发送函数（带重发机制）
 * @param data 待发送数据缓冲区指针
 * @param len  发送数据长度
 * @return true  发送成功
 * @return false 发送失败（重发5次后仍失败）
 * @note 1. 调用CDC_Transmit_FS进行底层发送
 *       2. 发送忙时自动重发，最多重试5次
 *       3. @warning 成功返回路径中trans_fault_tic清零语句位于return之后，实际不会执行
 */
bool fUSB_SendData(u8 *data, u8 len)
{
    if (BSP_USB_CDC_Transmit_FS(data, len)) // 发送成功
    {
        trans_fault_tic = 0; // 清除错误计数器
        return true;
    }
    else // 发送忙，启动重发机制
    {
        if (trans_fault_tic > 10) // 重发超过5次则放弃
        {
            trans_fault_tic = 0;
            return false;
        }
        trans_fault_tic++;
        fUSB_SendData(data, len); // 递归重发（注意：嵌入式环境需谨慎使用递归）
    }
    return false;
}

/**
 * @brief USB协议帧发送函数
 * @param id   消息ID
 * @param data 待发送数据指针
 * @param len  数据长度（1~MAX_USB_DATA_LEN）
 * @return true  帧封装并发送成功
 * @return false 发送失败或数据长度为0
 * @note 帧格式：头(1)+ID(1)+长度(1)+数据(N)+校验(1)+尾(1)
 *       校验算法：数据字节累加后取最低位
 */
bool fUSB_SendFrame(u8 id, u8 *data, u8 len)
{
    if (len == 0)
        return false;

    /* 计算校验值 */
    u16 check = 0;
    for (int i = 0; i < len; i++)
        check += data[i];

    /* 填充发送帧结构 */
    UsbTxFrame.id = id;
    UsbTxFrame.len = len;
    memcpy(UsbTxFrame.data, data, len);
    UsbTxFrame.check = (u8)check & 0xff;

    /* 组装完整帧：head(1)+id(1)+len(1)+data(N)+check(1)+tail(1) = N+5字节 */
    UsbTxFrame.data[len] = UsbTxFrame.check;    // 校验字节
    UsbTxFrame.data[len + 1] = UsbTxFrame.tail; // 帧尾

    /* 发送完整帧（包含帧头） */
    return fUSB_SendData((u8 *)&UsbTxFrame, len + 5);
}

/**
 * @brief USB接收帧数据处理回调（弱定义）
 * @param id   消息ID
 * @param data 接收数据缓冲区
 * @param len  有效数据长度
 * @note 用户可在应用层重写此函数实现业务逻辑
 */
__weak void fUSB_RxFrameCallback(u8 id, u8 *data, u8 len)
{
    return;
}

/**
 * @brief USB接收数据解析函数
 * @param data 指向接收数据缓冲区的指针
 * @param len  指向数据长度的指针（输入/输出参数）
 * @return true  帧解析成功（无论校验是否通过）
 * @return false 参数错误或校验失败
 * @note 1. 验证帧头尾完整性
 *       2. 校验数据域（调试模式可跳过校验）
 *       3. 校验通过后调用用户处理回调
 *       4. 帧格式：头(1)+ID(1)+长度(1)+数据(N)+校验(1)+尾(1)
 */
bool BSP_USB_RecvByte(u8 *data, u8 *len)
{
    /* 参数有效性检查 */
    if (data == NULL || len == NULL)
        return false;

    /* 验证帧头尾 */
    if ((data[0] == UsbRxFrame.head) && (data[*len - 1] == UsbRxFrame.tail))
    {
        /* 提取校验字节 */
        UsbRxFrame.check = data[*len - 2];

        /* 计算数据域校验和（从第3字节开始到校验字节前） */
        u16 check = 0;
        for (int i = 3; i < *len - 2; i++)
            check += data[i];

        /* 校验比对（取最低位） */
        if ((u8)(check & 0xff) != UsbRxFrame.check)
            return false; // 校验失败

        /* 校验通过，解析帧内容 */
        UsbRxFrame.id = data[1];                           // 消息ID
        UsbRxFrame.len = data[2];                          // 数据长度
        memcpy(UsbRxFrame.data, data + 3, UsbRxFrame.len); // 数据域

        /* 调用用户处理回调 */
        fUSB_RxFrameCallback(UsbRxFrame.id, UsbRxFrame.data, UsbRxFrame.len);
    }
    return true;
}