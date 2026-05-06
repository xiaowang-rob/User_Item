#ifndef __BSP_USB_H
#define __BSP_USB_H

#include "bsp.h"

/* ============================================
 * USB - USB虚拟串口
 * ============================================ */
void BSP_USB_CS(bool level);
bool BSP_USB_CDC_Transmit_FS(u8 *data, u16 len);
bool BSP_USB_RecvByte(u8 *data, u8 *len); // USB接收中断接口（供读取数据调用）

#endif /* __BSP_USB_H */