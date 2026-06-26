#ifndef __QUEUE_H
#define __QUEUE_H

#include "bsp_base.h"

// 队列状态枚举
typedef enum
{
    QUEUE_STATUS_OK = 0,
    QUEUE_STATUS_FULL,
    QUEUE_STATUS_EMPTY,
    QUEUE_STATUS_ERROR
} eQueueStatus;

// 静态队列结构体
typedef struct
{
    u8 *buffer;   // 数据缓冲区
    u16 front;    // 队首索引
    u16 rear;     // 队尾索引
    u16 capacity; // 队列容量
    u16 count;    // 当前元素数量
} tStaticQueue;

// 函数声明
eQueueStatus queue_static_init(tStaticQueue *queue, u8 *buffer, u16 capacity);
void queue_clear(tStaticQueue *queue);
bool queue_is_empty(const tStaticQueue *queue);
bool queue_is_full(const tStaticQueue *queue);
u16 queue_count(const tStaticQueue *queue);
u16 queue_remaining(const tStaticQueue *queue);
eQueueStatus queue_static_enqueue(tStaticQueue *queue, const u8 *data);
eQueueStatus queue_static_dequeue(tStaticQueue *queue, u8 *data);
eQueueStatus queue_static_peek(const tStaticQueue *queue, u8 *data);
eQueueStatus queue_static_peek_at(const tStaticQueue *queue, u16 index, void *data);

#endif // __QUEUE_H