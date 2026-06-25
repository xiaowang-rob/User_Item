#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include "bsp.h"

// ============================================
// CAN - 控制器局域网
// ============================================
bool bsp_can_init(u32 CAN_ID);
bool bsp_can_set_config(u32 CAN_ID);
bool bsp_can_send_data(u32 CAN_ID, u8 *msg, u8 len);
void bsp_can_rx_callback(bool *recv_ok, u32 *id, u8 *RxData, u32 *len); // CAN接收中断接口（供读取数据调用）

#endif // __BSP_CAN_H