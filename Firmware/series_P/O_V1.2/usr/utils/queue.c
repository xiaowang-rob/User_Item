#include "queue.h"
#include "string.h"
/**
 * @file queue.c
 * @brief 静态循环队列实现
 */

/* 内联函数定义 */
static inline bool bQueueIsNull(const tStaticQueue *queue)
{
    return (queue == NULL);
}

static inline bool bQueueIsEmptyInline(const tStaticQueue *queue)
{
    return bQueueIsNull(queue) ? true : (queue->count == 0);
}

static inline bool bQueueIsFullInline(const tStaticQueue *queue)
{
    return bQueueIsNull(queue) ? true : (queue->count == queue->capacity);
}

/**
 * @brief 初始化静态队列
 * @param queue 队列结构体指针
 * @param buffer 静态分配的缓冲区指针
 * @param capacity 队列最大容量（字节数）
 * @return eQueueStatus 操作结果状态
 */
eQueueStatus fStaticQueueInit(tStaticQueue *queue, u8 *buffer, u16 capacity)
{
    // 参数校验
    if (bQueueIsNull(queue) || buffer == NULL || capacity == 0)
    {
        return QUEUE_STATUS_ERROR;
    }

    // 初始化队列成员变量
    queue->buffer = buffer;
    queue->front = 0;
    queue->rear = 0;
    queue->capacity = capacity;
    queue->count = 0;

    return QUEUE_STATUS_OK;
}

/**
 * @brief 清空队列内容
 * @param queue 队列结构体指针
 */
void queue_clear(tStaticQueue *queue)
{
    if (bQueueIsNull(queue))
    {
        return;
    }

    // 重置队列状态
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
}

/**
 * @brief 检查队列是否为空
 * @param queue 队列结构体指针
 * @return bool 队列为空返回true，否则返回false
 */
bool queue_is_empty(const tStaticQueue *queue)
{
    return bQueueIsEmptyInline(queue);
}

/**
 * @brief 检查队列是否已满
 * @param queue 队列结构体指针
 * @return bool 队列已满返回true，否则返回false
 */
bool queue_is_full(const tStaticQueue *queue)
{
    return bQueueIsFullInline(queue);
}

/**
 * @brief 获取队列当前元素数量
 * @param queue 队列结构体指针
 * @return u16 当前队列中的元素数量
 */
u16 queue_count(const tStaticQueue *queue)
{
    if (bQueueIsNull(queue))
    {
        return 0;
    }
    return queue->count;
}

/**
 * @brief 获取队列剩余可用空间
 * @param queue 队列结构体指针
 * @return u16 队列剩余可用空间大小
 */
u16 queue_remaining(const tStaticQueue *queue)
{
    if (bQueueIsNull(queue))
    {
        return 0;
    }
    return (queue->capacity - queue->count);
}

/**
 * @brief 向队列末尾添加一个字节数据
 * @param queue 队列结构体指针
 * @param data 要添加的数据指针
 * @return eQueueStatus 操作结果状态
 */
eQueueStatus fStaticQueueEnqueue(tStaticQueue *queue, const u8 *data)
{
    // 参数校验和满队列检查
    if (bQueueIsNull(queue) || data == NULL)
    {
        return QUEUE_STATUS_ERROR;
    }

    if (bQueueIsFullInline(queue))
    {
        return QUEUE_STATUS_FULL;
    }

    // 数据入队
    queue->buffer[queue->rear] = *data;

    // 更新队尾指针和计数（循环队列处理）
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;

    return QUEUE_STATUS_OK;
}

/**
 * @brief 从队列头部取出一个字节数据
 * @param queue 队列结构体指针
 * @param data 接收数据的缓冲区指针（可为NULL）
 * @return eQueueStatus 操作结果状态
 */
eQueueStatus fStaticQueueDequeue(tStaticQueue *queue, u8 *data)
{
    // 参数校验和空队列检查
    if (bQueueIsNull(queue))
    {
        return QUEUE_STATUS_ERROR;
    }

    if (bQueueIsEmptyInline(queue))
    {
        return QUEUE_STATUS_EMPTY;
    }

    // 数据出队（可选复制到输出参数）
    if (data != NULL)
    {
        *data = queue->buffer[queue->front];
    }

    // 更新队首指针和计数（循环队列处理）
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;

    return QUEUE_STATUS_OK;
}

/**
 * @brief 查看队列头部元素（不移除）
 * @param queue 队列结构体指针
 * @param data 存储查看数据的缓冲区指针
 * @return eQueueStatus 操作结果状态
 */
eQueueStatus fStaticQueuePeek(const tStaticQueue *queue, u8 *data)
{
    // 参数校验和空队列检查
    if (bQueueIsNull(queue) || data == NULL)
    {
        return QUEUE_STATUS_ERROR;
    }

    if (bQueueIsEmptyInline(queue))
    {
        return QUEUE_STATUS_EMPTY;
    }

    // 直接复制队首数据
    *data = queue->buffer[queue->front];
    return QUEUE_STATUS_OK;
}

/**
 * @brief 查看队列中指定索引位置的元素（不移除）
 * @param queue 队列结构体指针
 * @param index 相对队首的索引位置（0表示队首）
 * @param data 存储查看数据的缓冲区指针
 * @return eQueueStatus 操作结果状态
 */
eQueueStatus fStaticQueuePeekAt(const tStaticQueue *queue, u16 index, void *data)
{
    // 参数校验和边界检查
    if (bQueueIsNull(queue) || data == NULL)
    {
        return QUEUE_STATUS_ERROR;
    }

    if (bQueueIsEmptyInline(queue))
    {
        return QUEUE_STATUS_EMPTY;
    }

    if (index >= queue->count)
    {
        return QUEUE_STATUS_ERROR;
    }

    // 计算实际缓冲区索引并复制数据
    u16 actual_index = (queue->front + index) % queue->capacity;
    *(u8 *)data = queue->buffer[actual_index];

    return QUEUE_STATUS_OK;
}