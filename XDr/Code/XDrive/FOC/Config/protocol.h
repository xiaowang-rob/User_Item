#ifndef __PROTOCOL_H
#define __PROTOCOL_H

/***********CMD ID***********/
#define USART_connect 0xf0    // 串口上位机连接
#define USART_disconnect 0xfe // 串口上位机断开

#define SYSTEM_DESC 0xff   // 系统描述
#define START_TUNNING 0xf1 // 开始调参
#define BRAKE 0xf2         // 刹车
#define FOC_NRST 0xf3      // FOC复位
#define ENABLE 0xf4        // 使能FOC
#define DISABLE 0xf5       // 失能FOC
#define PROTECT_RESET 0xf6 // 保护机制复位

#define LOG_GET 0xf7   // 获取日志
#define LOG_ERASE 0xf8 // 日志擦除

#define PARAM_ERASE 0x01 // 参数擦除
#define PARAM_WRITE 0x02 // 参数写入
#define PARAM_READ 0x03  // 参数读取
#define PARAM_SAVE 0x04  // 参数保存

#define CMD_REFVALUE_SET 0x21 // 参考值设置
#define CMD_MODE_SET 0x22     // 模式设置
#define CMD_STREAM_GET 0x23   // 监测值获取 单个值直接获取
#define CMD_STREAM_SET 0x25   // 要连续传输的数据流设置

#define MAX_frame_length 32 // 通讯帧最大长度
/***********USB Config***********/
/*
接收 ： 包头  ID（PC指令）  长度   数据（8bit)Index+Value     校验   包尾
发送 ： 包头  ID（PC指令）  长度   数据（PC查询数据）  校验   包尾
Index==0xff时，表示查询所有参数
*/
#define USB_PACKET_HEAD 0x2C
#define USB_PACKET_TAIL 0xC2
/***********UART Config***********/
// 默认和校验
// 115200bps
//[包头][ID][数据长度][数据域][和校验][包尾]
#define PACKET_HEAD 0x55
#define PACKET_TAIL 0xAA

/***********CAN Config***********/
/*
默认唯一扩展帧ID
频率1M
*/
#endif /* __PROTOCOL_H */