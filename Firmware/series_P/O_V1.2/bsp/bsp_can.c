#include "bsp_can.h"
#include "can.h"
#include "config.h"

// 超时

#define CAN_SEND_TIMEOUT_MS 1000
static u32 can_to_zero = 0;

// ============================================
// CAN - 控制器局域网
// ============================================
bool bsp_can_init(u32 CAN_ID)
{
    // 配置CAN过滤器
    CAN_FilterTypeDef sFilterConfig;
     sFilterConfig.FilterBank = 14;                     // CAN2专用过滤器组（14-27）
     sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // 掩码模式，精确匹配单个ID
     sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位过滤器尺度
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    // 计算标准帧ID在过滤器寄存器中的值：STID[10:0]位于[31:21]，IDE=0（标准帧），RTR=0（数据帧）
    u32 std_id = CAN_ID & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00;
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    // 设置掩码：匹配11位ID + IDE位(0) + RTR位(0)，其余位忽略
    u32 mask_reg = (STD_ID_MASK << 21) | 0x06;
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xFFFF;

    // 应用过滤器配置并更新设备状态
    if (HAL_CAN_ConfigFilter(&CAN_CH, &sFilterConfig) != HAL_OK)
    {
        return false;
    }

    // 启动CAN外设
    if (HAL_CAN_Start(&CAN_CH) != HAL_OK)
    {
        return false;
    }

    // 使能FIFO0接收中断
    if (HAL_CAN_ActivateNotification(&CAN_CH, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        return false;
    }
    return true;
}
bool bsp_can_set_config(u32 CAN_ID)
{
    // 停止CAN外设以安全重配置
    HAL_CAN_Stop(&CAN_CH);

    // 配置CAN过滤器（参数同初始化函数）
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 14;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    u32 std_id = CAN_ID & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00;
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    u32 mask_reg = (STD_ID_MASK << 21) | 0x06;
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xFFFF;

    // 应用新过滤器配置
    if (HAL_CAN_ConfigFilter(&CAN_CH, &sFilterConfig) != HAL_OK)
    {
        return false;
    }

    // 重启CAN外设
    if (HAL_CAN_Start(&CAN_CH) != HAL_OK)
    {
        return false;
    }

    // 重新使能接收中断
    if (HAL_CAN_ActivateNotification(&CAN_CH, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        return false;
    }
    return true;
}
bool bsp_can_send_data(u32 CAN_ID, u8 *msg, u8 len)
{
    u8 i = 0;
    u8 message[8];                    // 本地发送缓冲区（CAN数据帧最大8字节）
    u32 TxMailbox;                    // 实际使用的发送邮箱编号
     CAN_TxHeaderTypeDef CAN_TxHeader; // CAN发送帧头配置

    // 配置发送帧头参数
     CAN_TxHeader.StdId = CAN_ID;               // 标准帧ID（11位）
     CAN_TxHeader.ExtId = CAN_ID;               // 扩展帧ID（本实现未使用）
     CAN_TxHeader.IDE = CAN_ID_STD;             // 标准帧格式
     CAN_TxHeader.RTR = CAN_RTR_DATA;           // 数据帧（非远程帧）
     CAN_TxHeader.DLC = len;                    // 数据长度（0~8字节）
     CAN_TxHeader.TransmitGlobalTime = DISABLE; // 禁用时间戳

    // 复制用户数据到本地缓冲区
    for (i = 0; i < len; i++)
    {
        message[i] = msg[i];
    }

    // 将消息加入发送邮箱
    if (HAL_CAN_AddTxMessage(&CAN_CH, &CAN_TxHeader, message, &TxMailbox) != HAL_OK)
    {
        return false;
    }
     can_to_zero = bsp_get_tick(); // 记录发送开始时间
    // 阻塞等待发送完成（3个邮箱均空闲表示发送结束）
    while (HAL_CAN_GetTxMailboxesFreeLevel(&CAN_CH) < 3)
    {
        u32 timeout = bsp_get_tick() - can_to_zero; // 获取当前时间
        if (timeout > CAN_SEND_TIMEOUT_MS)
            return false; // 超时

        // 空循环等待
    }

    return true;
}

__weak void bsp_can_rx_callback(bool *recv_ok, u32 *id, u8 *RxData, u32 *len) // CAN接收中断接口（供读取数据调用）
{
    return;
}

void bsp_can_rx_fifo0_msg_pending_callback(CAN_HandleTypeDef *hcan)
{
    u8 RxData[8];                     // 接收数据缓冲区
     CAN_RxHeaderTypeDef CAN_RxHeader; // 接收帧头信息
    bool recv_ok = false;

    // 从FIFO0读取消息
    recv_ok = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_RxHeader, RxData) == HAL_OK;
    bsp_can_rx_callback(&recv_ok, &CAN_RxHeader.StdId, RxData, &CAN_RxHeader.DLC);
}
