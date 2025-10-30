#include "can.h"
#include "canDr.h"

u8 can_Status = 0;
u8 can_fault_tic = 0;

#define CAN_FARME_LEN 5 // Id + 4bitData
u32 can_id = 0x00;      // CAN ID
bool CAN_queue = false;
static u8 CanRxData[64];
StaticQueue CAN_rx_queue;

u8 CAN_STATE_get()
{
    return can_Status;
}

__weak void CAN_RxData_Deal(u8 *RxData, u8 len)
{
    return;
}
/**
 * @brief  CAN过滤器配置和初始化函数
 * @note   配置CAN2的过滤器，启动CAN2外设，并使能接收中断
 */

void CANDr_Init(u32 CAN_ID, bool canQUEUE)
{
    can_id = CAN_ID;
    CAN_queue = canQUEUE;
    CAN_FilterTypeDef sFilterConfig;

    /* Configure the CAN Filter - 配置CAN过滤器 */
    sFilterConfig.FilterBank = 0;                                // 过滤器编号，使用一个CAN，则可选0-13；使用两个CAN可选0-27
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;            // 过滤器模式，掩码模式或列表模式
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;           // 过滤器位宽，32位模式（支持标准帧和扩展帧）
    sFilterConfig.FilterIdHigh = ((can_id << 5) >> 16) & 0xFFFF; // 过滤器验证码ID高16位，0-0xFFFF
    sFilterConfig.FilterIdLow = (can_id << 5) & 0xFFFF;          // 过滤器验证码ID低16位，0-0xFFFF
    sFilterConfig.FilterMaskIdHigh = 0x0000;                     // 过滤器掩码ID高16位，0-0xFFFF
    sFilterConfig.FilterMaskIdLow = 0x0000;                      // 过滤器掩码ID低16位，0-0xFFFF
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;           // FIFOx，0或1，指定接收消息存入FIFO0
    sFilterConfig.FilterActivation = ENABLE;                     // 使能过滤器
    sFilterConfig.SlaveStartFilterBank = 14;                     // 从过滤器编号，0-27，对于单CAN实例该参数没有意义

    can_Status = 0;
    // 配置CAN2的过滤器参数
    if (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        can_Status = 1;
    }

    /* Start the CAN peripheral - 启动CAN外设 */
    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
        can_Status = 1;
    }

    /* Activate CAN RX notification - 激活CAN接收中断通知 */
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        can_Status = 1;
    }
    if (CAN_queue)
        static_queue_init(&CAN_rx_queue, CanRxData, 64);
}

/**
 * @brief  CAN发送消息函数
 * @param  msg: 要发送的数据指针
 * @param  len: 要发送的数据长度（最大8字节）
 * @retval 0: 发送成功
 * @retval 1: 发送失败
 * @note   使用标准帧模式，ID为0x12，发送阻塞直到发送完成
 */
bool CAN_Send_Msg(u8 *msg, u8 len)
{
    u8 i = 0;
    u8 message[8];                    // 临时存储要发送的数据（CAN数据帧最大8字节）
    u32 TxMailbox;                    // 发送邮箱编号
    CAN_TxHeaderTypeDef CAN_TxHeader; // CAN发送头结构体

    // 设置发送参数
    CAN_TxHeader.StdId = can_id;               // 标准标识符(11位)
    CAN_TxHeader.ExtId = can_id;               // 扩展标识符(29位)
    CAN_TxHeader.IDE = CAN_ID_STD;             // 使用标准帧（11位ID），如果改为CAN_ID_EXT则使用扩展帧
    CAN_TxHeader.RTR = CAN_RTR_DATA;           // 发送数据帧（不是远程帧）
    CAN_TxHeader.DLC = len;                    // 数据长度，0-8字节
    CAN_TxHeader.TransmitGlobalTime = DISABLE; // 禁用全局时间戳

    // 装载数据 - 将输入的数据复制到本地数组
    for (i = 0; i < len; i++)
    {
        message[i] = msg[i]; // 复制数据到本地缓冲区
    }

    // 发送CAN消息 - 将消息添加到发送邮箱
    if (HAL_CAN_AddTxMessage(&hcan2, &CAN_TxHeader, message, &TxMailbox) != HAL_OK)
    {
        can_Status = 2;
    }

    // 等待发送完成 - 等待所有发送邮箱都空闲（表示消息已发送）
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) < 3) // 3个发送邮箱都空闲时返回3
    {
    }
    return true;
}

/**
 * @brief  CAN接收FIFO0消息挂起中断回调函数
 * @param  hcan: CAN句柄指针
 * @note   当CAN接收FIFO0中有新消息时，硬件会自动调用此函数
 *         这是HAL库提供的弱定义函数，用户可以重写实现自己的处理逻辑
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    u8 RxData[8];                     // 接收数据缓冲区，最大8字节
    CAN_RxHeaderTypeDef CAN_RxHeader; // CAN接收头结构体

    // 从FIFO0中获取接收的消息
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_RxHeader, RxData) == HAL_OK)
    {
        // CAN_RxHeader 包含以下信息：
        // - CAN_RxHeader.StdId: 标准ID（如果接收的是标准帧）
        // - CAN_RxHeader.ExtId: 扩展ID（如果接收的是扩展帧）
        // - CAN_RxHeader.IDE: 帧格式（标准帧/扩展帧）
        // - CAN_RxHeader.DLC: 数据长度
        // - CAN_RxHeader.RTR: 远程传输请求标志

        // RxData 数组包含接收到的8字节数据
        // 示例：可以根据ID判断消息类型，然后处理相应的数据
        if (CAN_RxHeader.StdId == can_id)
        {
            if (CAN_queue)
            {
                static_queue_enqueue(&CAN_rx_queue, 0xE5);
                for (int i = 0; i < CAN_RxHeader.DLC; i++)
                    static_queue_enqueue(&CAN_rx_queue, RxData);
                static_queue_enqueue(&CAN_rx_queue, 0x5E);
            }
            else
                CAN_RxData_Deal(RxData, CAN_RxHeader.DLC);
        }
        if (can_fault_tic >= 1)
            can_fault_tic--;
    }
    else
        can_fault_tic += 5;
    if (can_fault_tic >= 50)
        can_Status = 2;
    else if (can_Status == 2)
        can_Status = 0;
}