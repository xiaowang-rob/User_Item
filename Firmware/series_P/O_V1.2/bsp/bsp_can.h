#ifndef __BSP_CAN_H
#define __BSP_CAN_H

#include "bsp.h"

/* ============================================
 * CAN - 控制器局域网
 * ============================================ */
bool BSP_CanInit(u32 CAN_ID);
bool BSP_CanSetConfig(u32 CAN_ID);
bool BSP_CanSendData(u32 CAN_ID, u8 *msg, u8 len);
void bsp_can_rx_callback(bool *recv_ok, u32 *id, u8 *RxData, u32 *len); // CAN接收中断接口（供读取数据调用）

#endif /* __BSP_CAN_H */