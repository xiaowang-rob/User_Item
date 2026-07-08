#ifndef __BSP_USB_H
#define __BSP_USB_H

#include "bsp_base.h"

// ============================================
// USB - USB虚拟串口
// ============================================
void bsp_usb_cs(bool level);
bool bsp_usb_cdc_transmit_fs(u8 *data, u16 len);
bool bsp_usb_recv_byte(u8 *data, u8 *len); // USB接收中断接口（供读取数据调用）

#endif // __BSP_USB_H