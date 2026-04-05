#ifndef __PROTOCOL_H
#define __PROTOCOL_H

/***********CMD ID***********/
#define UC_connect 0xf0    // 上位机连接
#define UC_disconnect 0xfe // 上位机断开

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

#define CMD_REFVALUE_SET 0x21  // 目标值设置
#define CMD_MODE_SET 0x22      // 模式设置
#define CMD_STREAM_GET 0x23    // 监测值获取 单个值直接获取
#define CMD_STREAM_SET 0x25    // 要连续传输的数据流设置
#define CMD_SET_ZERO_POS 0x26  // 设置零点
#define CMD_SET_LIMIT_POS 0x27 // 设置极限位置

#define CMD_HANDSHAKE 0x28 // CAN握手

#define CMD_SYSTEM_RESET 0x30 // 系统复位

#define CMD_IAP_ENTER 0x31 // 进入IAP模式 进入开始固件烧录确认
/* 这些在上位机 和 bl上实现
#define CMD_IAP_ERASE_FLASH 0x32  // 擦除flash
#define CMD_IAP_WRITE_FLASH 0x33  // 写入flash
#define CMD_IAP_VERIFY_FLASH 0x34 // 校验flash
#define CMD_IAP_EXIT 0x35         // 完成 退出IAP模式 进入APP
*/
#define MAX_frame_length 128 // 通讯帧最大长度

// 这个在数据段
#define FEEDBACK_OK 0xf0
#define FEEDBACK_ERROR 0xfe
/***********USB Config***********/
/*
接收 ： 包头  ID（PC指令）  长度   数据（8bit)Index+Value     校验   包尾
发送 ： 包头  ID（PC指令）  长度   数据（PC查询数据）  校验   包尾
Index==0xff时，表示查询所有参数
*/
#define USB_PACKET_HEAD 0x3A
#define USB_PACKET_TAIL 0x0D
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