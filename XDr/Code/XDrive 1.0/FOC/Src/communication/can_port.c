#include "can_port.h"
#include "protocol.h"
#include "can.h"
#include "device.h"

#define STD_ID_MASK 0x7FF      // 11bit [31：21] 32位模式下 [15:5] 16位模式下
#define EXT_ID_MASK 0xFFFFFFFF // 29bit [31:3]

static u8 idindex;
static u8 txbuffer[8];
static u8 rxtemp;
static u8 rxbuffer[8];
static u8 rxindex;
static u8 CanRxData[64];

CAN_Handle_t can = {.queue_head = 0xE5, .queue_tail = 0x5E};

eQueueStatus CAN_deQUEUE_data(u8 *data)
{
    return fStaticQueueDequeue(&can.rx_queue, data);
}

/**
 * @brief  CAN过滤器配置和初始化函数
 * @note   配置CAN2的过滤器，启动CAN2外设，并使能接收中断
 */
u8 can_init_ok = 0x12;

void CAN_PORT_Init(u32 CAN_ID, bool canQUEUE)
{

    can.id = CAN_ID;
    can.queue_flag = canQUEUE;

    /* Configure the CAN Filter - 配置CAN过滤器 */
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = 14;                     // CAN2使用14-27
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // 掩码模式（更适合单个ID匹配）
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位模式
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    // 计算标准帧ID的过滤器寄存器值
    u32 std_id = can.id & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00; // STID[10:0]在[31:21], IDE=0, RTR=0

    // 设置要匹配的ID
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    // 设置掩码：匹配ID的11位 + IDE位 + RTR位
    // 0x7FF << 21 = 匹配所有11位ID
    // 0x04 = 匹配IDE位（必须为0）
    // 0x02 = 匹配RTR位（必须为0）
    u32 mask_reg = (STD_ID_MASK << 21) | 0x06; // 匹配ID、IDE和RTR位
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xffff;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xffff;

    // 配置过滤器
    if (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }
    else
    {
        g_device_status.can_state = ONLINE;
    }

    /* Start the CAN peripheral - 启动CAN外设 */
    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    /* Activate CAN RX notification - 激活CAN接收中断通知 */
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }
    // 如果开启了队列模式则初始化队列
    if (can.queue_flag)
        if (QUEUE_STATUS_ERROR == fStaticQueueInit(&can.rx_queue, CanRxData, sizeof(CanRxData)))
            g_device_status.can_state = OFFLINE;

    CAN_Send_Msg(&can_init_ok, 1);
}

void CAN_SET_ID_QUEUE(u32 CAN_ID, bool canQUEUE)
{
    // 停止CAN2
    HAL_CAN_Stop(&hcan2);

    can.id = CAN_ID;
    can.queue_flag = canQUEUE;

    /* Configure the CAN Filter - 配置CAN过滤器 */
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = 14;                     // CAN2使用14-27
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // 掩码模式（更适合单个ID匹配）
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位模式
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    // 计算标准帧ID的过滤器寄存器值
    u32 std_id = can.id & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00; // STID[10:0]在[31:21], IDE=0, RTR=0

    // 设置要匹配的ID
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    // 设置掩码：匹配ID的11位 + IDE位 + RTR位
    // 0x7FF << 21 = 匹配所有11位ID
    // 0x04 = 匹配IDE位（必须为0）
    // 0x02 = 匹配RTR位（必须为0）
    u32 mask_reg = (STD_ID_MASK << 21) | 0x06; // 匹配ID、IDE和RTR位
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xffff;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xffff;

    // 配置过滤器
    if (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }
    else
    {
        g_device_status.can_state = ONLINE;
    }

    // 重新启动CAN2
    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    // 重新激活接收中断
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    if (can.queue_flag)
        if (QUEUE_STATUS_ERROR == fStaticQueueInit(&can.rx_queue, CanRxData, sizeof(CanRxData)))
            g_device_status.can_state = OFFLINE;
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
    CAN_TxHeader.StdId = can.id;               // 标准标识符(11位)
    CAN_TxHeader.ExtId = can.id;               // 扩展标识符(29位)
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
        g_device_status.can_state = RUN_ERROR;
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
        if (CAN_RxHeader.StdId == can.id)
        {
            if (can.queue_flag)
            {
                fStaticQueueEnqueue(&can.rx_queue, &can.queue_head);
                for (int i = 0; i < CAN_RxHeader.DLC; i++)
                    fStaticQueueEnqueue(&can.rx_queue, RxData);
                fStaticQueueEnqueue(&can.rx_queue, &can.queue_tail);
            }
            else
                CAN_RxData_Deal(RxData, CAN_RxHeader.DLC);
        }
        if (can.err_count >= 1)
            can.err_count--;
    }
    else
        can.err_count += 5;
    if (can.err_count >= 50)
        g_device_status.can_state = RUN_ERROR;
    else
        g_device_status.can_state = RUNNING;
}
/*
控制 4byte和pvt 8byte
模式设置 2byte
模式/参数查询 3byte
数据流查询 2byte
使能 1byte
失能 1byte
*/
__weak void CAN_RxData_Deal(u8 *RxData, u8 len)
{
}
static bool _get_head = false;
void CAN_data_byte_deal(u8 data)
{
    if (_get_head)
    {
        if (rxindex < 8)
        {
            if (data == 0x5E)
            {
                CAN_RxData_Deal(rxbuffer, rxindex + 1);
                rxindex = 0;
                _get_head = false;
            }
            else
                rxbuffer[rxindex++] = data;
        }
        else
            _get_head = false;
    }
    else if (data == 0xE5)
        _get_head = true;
}
void CAN_QUEUE_Deal()
{
    while (CAN_deQUEUE_data(&rxtemp) == QUEUE_STATUS_OK)
    {
        CAN_data_byte_deal(rxtemp);
    }
}
