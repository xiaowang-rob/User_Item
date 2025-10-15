#include "can.h"
#include "canDr.h"

// todo:再好好学习一下CAN协议，然后再写这个驱动
/**
 * @brief  CAN过滤器配置和初始化函数
 * @note   配置CAN2的过滤器，启动CAN2外设，并使能接收中断
 */
void CANDr_Init(void)
{
    CAN_FilterTypeDef sFilterConfig;

    /* Configure the CAN Filter - 配置CAN过滤器 */
    sFilterConfig.FilterBank = 0;                      // 过滤器编号，使用一个CAN，则可选0-13；使用两个CAN可选0-27
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // 过滤器模式，掩码模式或列表模式
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 过滤器位宽，32位模式（支持标准帧和扩展帧）
    sFilterConfig.FilterIdHigh = 0x0000;               // 过滤器验证码ID高16位，0-0xFFFF
    sFilterConfig.FilterIdLow = 0x0000;                // 过滤器验证码ID低16位，0-0xFFFF
    sFilterConfig.FilterMaskIdHigh = 0x0000;           // 过滤器掩码ID高16位，0-0xFFFF
    sFilterConfig.FilterMaskIdLow = 0x0000;            // 过滤器掩码ID低16位，0-0xFFFF
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // FIFOx，0或1，指定接收消息存入FIFO0
    sFilterConfig.FilterActivation = ENABLE;           // 使能过滤器
    sFilterConfig.SlaveStartFilterBank = 14;           // 从过滤器编号，0-27，对于单CAN实例该参数没有意义

    // 配置CAN2的过滤器参数
    if (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        User_Error_Handler(1); // 配置失败，进入错误处理函数
    }

    /* Start the CAN peripheral - 启动CAN外设 */
    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
        User_Error_Handler(1); // 启动失败，进入错误处理函数
    }

    /* Activate CAN RX notification - 激活CAN接收中断通知 */
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        User_Error_Handler(1); // 激活中断失败，进入错误处理函数
    }
}

/**
 * @brief  CAN发送消息函数
 * @param  msg: 要发送的数据指针
 * @param  len: 要发送的数据长度（最大8字节）
 * @retval 0: 发送成功
 * @retval 1: 发送失败
 * @note   使用标准帧模式，ID为0x12，发送阻塞直到发送完成
 */
u8 CAN_Send_Msg(u8 *msg, u8 len)
{
    u8 i = 0;
    u8 message[8];                    // 临时存储要发送的数据（CAN数据帧最大8字节）
    u32 TxMailbox;                    // 发送邮箱编号
    CAN_TxHeaderTypeDef CAN_TxHeader; // CAN发送头结构体

    // 设置发送参数
    CAN_TxHeader.StdId = 0x12;                 // 标准标识符(11位)，值为0x12
    CAN_TxHeader.ExtId = 0x12;                 // 扩展标识符(29位)，值为0x12
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
        return 1; // 添加到发送邮箱失败，返回错误码1
    }

    // 等待发送完成 - 等待所有发送邮箱都空闲（表示消息已发送）
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) != 3) // 3个发送邮箱都空闲时返回3
    {
        // 空循环等待发送完成
        // 注意：这里是一个阻塞等待，会一直占用CPU直到发送完成
    }

    return 0; // 发送成功，返回0
}

/**
 * @brief  CAN接收FIFO0消息挂起中断回调函数
 * @param  hcan: CAN句柄指针
 * @note   当CAN接收FIFO0中有新消息时，硬件会自动调用此函数
 *         这是HAL库提供的弱定义函数，用户可以重写实现自己的处理逻辑
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 收到CAN数据会触发接收中断，进入该回调函数

    u32 i;                            // 循环计数器
    u8 RxData[8];                     // 接收数据缓冲区，最大8字节
    CAN_RxHeaderTypeDef CAN_RxHeader; // CAN接收头结构体

    // 从FIFO0中获取接收的消息
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_RxHeader, RxData) == HAL_OK)
    {
        // 接收消息成功，现在可以处理接收到的数据

        // CAN_RxHeader 包含以下信息：
        // - CAN_RxHeader.StdId: 标准ID（如果接收的是标准帧）
        // - CAN_RxHeader.ExtId: 扩展ID（如果接收的是扩展帧）
        // - CAN_RxHeader.IDE: 帧格式（标准帧/扩展帧）
        // - CAN_RxHeader.DLC: 数据长度
        // - CAN_RxHeader.RTR: 远程传输请求标志

        // RxData 数组包含接收到的8字节数据

        // todo: 处理接收到的数据
        // 示例：可以根据ID判断消息类型，然后处理相应的数据
        // if(CAN_RxHeader.StdId == 0x12) {
        //     // 处理ID为0x12的消息
        //     ProcessData(RxData, CAN_RxHeader.DLC);
        // }

        // 可以在这里添加你的数据处理逻辑
        // 比如：解析协议、更新状态、触发事件等
    }
    // 如果接收失败，函数直接返回，不会执行处理逻辑
}