/**
 * @file can_port.c
 * @brief CAN端口驱动实现文件
 * @details 包含CAN初始化、动态配置、数据收发及队列处理等功能
 */

#include "can_port.h"
#include "protocol.h"
#include "can.h"
#include "device.h"

#define STD_ID_MASK 0x7FF      // 标准帧11位ID掩码 [31:21]（32位模式）或 [15:5]（16位模式）
#define EXT_ID_MASK 0xFFFFFFFF // 扩展帧29位ID掩码 [31:3]

/* CAN接收解析状态变量 */
static u8 rxtemp;
static u8 rxbuffer[8];
static u8 rxindex;
static u8 _get_head = false;
static u8 CanRxData[64];

/* CAN句柄全局变量，包含队列头尾标识符 */
tCAN_handle can = {.queue_head = 0xE5, .queue_tail = 0x5E};

/**
 * @brief  CAN端口初始化函数
 * @param  CAN_ID 本节点的CAN标准帧ID（11位）
 * @param  canQUEUE 是否启用接收队列模式（true:启用, false:禁用）
 * @note   1. 配置CAN2过滤器（Bank 14），仅接收指定ID的标准数据帧
 *         2. 启动CAN2外设并使能FIFO0接收中断
 *         3. 若启用队列模式，初始化静态接收队列
 *         4. 初始化完成后自动发送一条执行指令
 */
void fCAN_PortInit(u32 CAN_ID, bool canQUEUE)
{
    can.id = CAN_ID;
    can.queue_flag = canQUEUE;

    /* 配置CAN过滤器 */
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 14;                     // CAN2专用过滤器组（14-27）
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // 掩码模式，精确匹配单个ID
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位过滤器尺度
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    /* 计算标准帧ID在过滤器寄存器中的值：STID[10:0]位于[31:21]，IDE=0（标准帧），RTR=0（数据帧） */
    u32 std_id = can.id & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00;
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    /* 设置掩码：匹配11位ID + IDE位(0) + RTR位(0)，其余位忽略 */
    u32 mask_reg = (STD_ID_MASK << 21) | 0x06;
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xFFFF;

    /* 应用过滤器配置并更新设备状态 */
    if (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }
    else
    {
        g_device_status.can_state = ONLINE;
    }

    /* 启动CAN外设 */
    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    /* 使能FIFO0接收中断 */
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    /* 初始化接收队列（若启用） */
    if (can.queue_flag)
    {
        if (QUEUE_STATUS_ERROR == fStaticQueueInit(&can.rx_queue, CanRxData, sizeof(CanRxData)))
        {
            g_device_status.can_state = OFFLINE;
        }
    }

    /* 初始化完成后发送执行指令 */
    fCAN_SendData((u8 *)&execute, 1);
}

/**
 * @brief  动态重新配置CAN参数
 * @param  CAN_ID 新的CAN标准帧ID（11位）
 * @param  canQUEUE 是否启用接收队列模式（true:启用, false:禁用）
 * @note   1. 先停止CAN2外设，避免配置过程中产生异常中断
 *         2. 重新配置过滤器参数，支持运行时修改节点ID
 *         3. 重启CAN外设并恢复中断使能
 *         4. 队列模式切换时重新初始化静态队列
 */
void fCAN_SetConfig(u32 CAN_ID, bool canQUEUE)
{
    /* 停止CAN外设以安全重配置 */
    HAL_CAN_Stop(&hcan2);

    can.id = CAN_ID;
    can.queue_flag = canQUEUE;

    /* 配置CAN过滤器（参数同初始化函数） */
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 14;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    u32 std_id = can.id & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00;
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    u32 mask_reg = (STD_ID_MASK << 21) | 0x06;
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xFFFF;

    /* 应用新过滤器配置 */
    if (HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }
    else
    {
        g_device_status.can_state = ONLINE;
    }

    /* 重启CAN外设 */
    if (HAL_CAN_Start(&hcan2) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    /* 重新使能接收中断 */
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        g_device_status.can_state = OFFLINE;
    }

    /* 重新初始化接收队列（若启用） */
    if (can.queue_flag)
    {
        if (QUEUE_STATUS_ERROR == fStaticQueueInit(&can.rx_queue, CanRxData, sizeof(CanRxData)))
        {
            g_device_status.can_state = OFFLINE;
        }
    }
}

/**
 * @brief  CAN数据发送函数
 * @param  msg 指向待发送数据缓冲区的指针
 * @param  len 待发送数据长度（1~8字节）
 * @return true  发送操作成功提交
 * @return false 发送操作失败（当前实现始终返回true，错误通过状态机上报）
 * @note   1. 使用标准帧格式，ID为初始化时设置的can.id
 *         2. 采用阻塞方式等待发送完成（检测所有发送邮箱空闲）
 *         3. 发送失败时更新设备状态为RUN_ERROR，但函数仍返回true以保持接口简洁
 */
bool fCAN_SendData(u8 *msg, u8 len)
{
    u8 i = 0;
    u8 message[8];                    // 本地发送缓冲区（CAN数据帧最大8字节）
    u32 TxMailbox;                    // 实际使用的发送邮箱编号
    CAN_TxHeaderTypeDef CAN_TxHeader; // CAN发送帧头配置

    /* 配置发送帧头参数 */
    CAN_TxHeader.StdId = can.id;               // 标准帧ID（11位）
    CAN_TxHeader.ExtId = can.id;               // 扩展帧ID（本实现未使用）
    CAN_TxHeader.IDE = CAN_ID_STD;             // 标准帧格式
    CAN_TxHeader.RTR = CAN_RTR_DATA;           // 数据帧（非远程帧）
    CAN_TxHeader.DLC = len;                    // 数据长度（0~8字节）
    CAN_TxHeader.TransmitGlobalTime = DISABLE; // 禁用时间戳

    /* 复制用户数据到本地缓冲区 */
    for (i = 0; i < len; i++)
    {
        message[i] = msg[i];
    }

    /* 将消息加入发送邮箱 */
    if (HAL_CAN_AddTxMessage(&hcan2, &CAN_TxHeader, message, &TxMailbox) != HAL_OK)
    {
        g_device_status.can_state = RUN_ERROR;
    }

    /* 阻塞等待发送完成（3个邮箱均空闲表示发送结束） */
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) < 3)
    {
        // 空循环等待
    }

    return true;
}

/**
 * @brief  CAN接收数据处理回调函数（弱定义）
 * @param  RxData 指向接收到的数据缓冲区的指针
 * @param  len    接收到的有效数据长度（1~8字节）
 * @note   1. 本函数为弱定义，用户可在应用层重写实现自定义解析逻辑
 *         2. 默认为空实现，需用户根据协议填充处理代码
 *         3. 非队列模式下，此函数由中断直接调用
 */
__weak void fCAN_RxDataCallback(u8 *RxData, u8 len)
{
    // 用户重写此函数以实现具体协议解析
}

/**
 * @brief  CAN FIFO0接收中断回调函数
 * @param  hcan CAN外设句柄指针
 * @note   1. 由HAL库在FIFO0收到新消息时自动触发
 *         2. 仅处理与本节点ID匹配的标准数据帧
 *         3. 根据队列模式选择：直接调用回调函数 或 存入静态队列
 *         4. 维护错误计数器，连续错误超限则置设备为RUN_ERROR状态
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    u8 RxData[8];                     // 接收数据缓冲区
    CAN_RxHeaderTypeDef CAN_RxHeader; // 接收帧头信息

    /* 从FIFO0读取消息 */
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_RxHeader, RxData) == HAL_OK)
    {
        /* 仅处理匹配本节点ID的标准数据帧 */
        if (CAN_RxHeader.StdId == can.id)
        {
            if (can.queue_flag)
            {
                /* 队列模式：按协议封装（头+数据+尾）存入队列 */
                fStaticQueueEnqueue(&can.rx_queue, &can.queue_head);
                for (int i = 0; i < CAN_RxHeader.DLC; i++)
                {
                    fStaticQueueEnqueue(&can.rx_queue, &RxData[i]);
                }
                fStaticQueueEnqueue(&can.rx_queue, &can.queue_tail);
            }
            else
            {
                /* 非队列模式：直接调用用户回调函数 */
                fCAN_RxDataCallback(RxData, CAN_RxHeader.DLC);
            }
        }

        /* 错误计数器衰减（正常接收时降低错误计数） */
        if (can.err_count >= 1)
        {
            can.err_count--;
        }
    }
    else
    {
        /* 接收失败：增加错误计数 */
        can.err_count += 5;
    }

    /* 根据错误计数更新设备状态 */
    if (can.err_count >= 50)
    {
        g_device_status.can_state = RUN_ERROR;
    }
    else
    {
        g_device_status.can_state = RUNNING;
    }
}

/**
 * @brief  CAN队列数据字节解析函数
 * @param  data 待解析的单字节数据
 * @note   1. 实现简单帧协议解析：0xE5为帧头，0x5E为帧尾，中间最多8字节有效数据
 *         2. 用于队列模式下重组被拆分为字节流的CAN帧
 *         3. 成功解析一帧后自动调用fCAN_RxDataDeal进行业务处理
 */
void CAN_data_byte_deal(u8 data)
{
    if (_get_head)
    {
        if (rxindex < 8)
        {
            /* 检测帧尾标志 */
            if (data == 0x5E)
            {
                /* 完成一帧解析，提交数据处理 */
                fCAN_RxDataCallback(rxbuffer, rxindex);
                rxindex = 0;
                _get_head = false;
            }
            else
            {
                /* 存储有效数据字节 */
                rxbuffer[rxindex++] = data;
            }
        }
        else
        {
            /* 超过最大数据长度，丢弃当前帧 */
            _get_head = false;
        }
    }
    else if (data == 0xE5)
    {
        /* 检测到帧头，开始新帧解析 */
        _get_head = true;
        rxindex = 0;
    }
}

/**
 * @brief  处理CAN接收队列中的数据
 * @note   1. 从静态队列中逐字节取出数据
 *         2. 调用CAN_data_byte_deal进行帧重组与解析
 *         3. 应在主循环或低优先级任务中周期调用，避免中断中处理耗时操作
 */
void fCAN_QueueData_deal(void)
{
    while (fStaticQueueDequeue(&can.rx_queue, &rxtemp) == QUEUE_STATUS_OK)
    {
        CAN_data_byte_deal(rxtemp);
    }
}