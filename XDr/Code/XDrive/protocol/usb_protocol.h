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

#define GET_SYSTEM_DESC 0x01
#define PARAMETER_erase 0x02
#define PARAMETER_read 0x03
#define PARAMETER_write 0x04
#define PARAMETER_save 0x05
#define CONTROL_VALUE_write 0x10
#define CONTROL_MODE_write 0x11
#define STREAM_read 0x12
#define STATUS_read 0x13

#endif // USB_PROTOCOL_H