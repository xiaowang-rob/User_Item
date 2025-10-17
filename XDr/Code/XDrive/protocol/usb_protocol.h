#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

/*usbd_desc.h*/
/*
#define USBD_VID 1155
#define USBD_LANGID_STRING 1033
#define USBD_MANUFACTURER_STRING "STMicroelectronics"
#define USBD_PID_FS 22336
#define USBD_PRODUCT_STRING_FS "XDrive"
#define USBD_CONFIGURATION_STRING_FS "CDC Config"
#define USBD_INTERFACE_STRING_FS "CDC Interface"

#define USB_SIZ_BOS_DESC 0x0C
*/

/*
接收 ： 包头  ID（PC指令）  长度   数据（8bit)Index+Value     校验   包尾
发送 ： 包头  ID（PC指令）  长度   数据（PC查询数据）  校验   包尾
Index==0xff时，表示查询所有参数
*/
#define USB_PACKET_HEAD 0x2C
#define USB_PACKET_TAIL 0xC2
#define USB_PACKET_MAX_SIZE 256

#define USB_connect 0xff         // 连接
#define GET_SYSTEM_DESC 0x01     // 获取系统描述符
#define PARAMETER_erase 0x02     // 擦除参数
#define PARAMETER_read 0x03      // 读取参数
#define PARAMETER_write 0x04     // 写入参数
#define PARAMETER_save 0x05      // 保存参数
#define CONTROL_VALUE_write 0x10 // 控制值写入
#define CONTROL_MODE_write 0x11  // 控制模式写入
#define STREAM_read 0x12         // 读取流数据
#define STATUS_read 0x13         // 读取状态
#define START_TUNNING 0x20       // 开始调参
#define BREAK_STOP 0x21          // 紧急停止
#define CLEAR_ERROR 0x22         // 清除错误
#define READ_LOG 0x23            // 读取日志

#endif // USB_PROTOCOL_H