#ifndef __QUEUE_H
#define __QUEUE_H

#include "main.h"

/* 队列状态枚举 */
typedef enum
{
    QUEUE_OK = 0,
    QUEUE_FULL,
    QUEUE_EMPTY,
    QUEUE_ERROR
} QueueStatus;

/* 静态队列结构体 */
typedef struct
{
    u8 *buffer;   // 数据缓冲区
    u16 front;    // 队首索引
    u16 rear;     // 队尾索引
    u16 capacity; // 队列容量
    u16 count;    // 当前字节数量
} StaticQueue;

/* 函数声明 */

/**
 * @brief 初始化静态队列
 * @param queue 队列指针
 * @param buffer 静态缓冲区
 * @param capacity 队列容量
 * @param item_size 每个元素的大小
 * @return 初始化状态
 */
QueueStatus static_queue_init(StaticQueue *queue, u8 *buffer,
                              u16 capacity);

/**
 * @brief 清空队列
 * @param queue 队列指针
 */
void static_queue_clear(StaticQueue *queue);

/**
 * @brief 检查队列是否为空
 * @param queue 队列指针
 * @return true-空, false-非空
 */
bool static_queue_is_empty(const StaticQueue *queue);

/**
 * @brief 检查队列是否已满
 * @param queue 队列指针
 * @return true-满, false-非满
 */
bool static_queue_is_full(const StaticQueue *queue);

/**
 * @brief 获取队列当前元素数量
 * @param queue 队列指针
 * @return 元素数量
 */
u16 static_queue_count(const StaticQueue *queue);

/**
 * @brief 获取队列剩余空间
 * @param queue 队列指针
 * @return 剩余空间（元素个数）
 */
u16 static_queue_remaining(const StaticQueue *queue);

/**
 * @brief 入队操作
 * @param queue 队列指针
 * @param data 要入队的数据指针
 * @return 操作状态
 */
QueueStatus static_queue_enqueue(StaticQueue *queue, const u8 *data);

/**
 * @brief 出队操作
 * @param queue 队列指针
 * @param data 出队数据存储位置（可为NULL）
 * @return 操作状态
 */
QueueStatus static_queue_dequeue(StaticQueue *queue, u8 *data);

/**
 * @brief 查看队首元素
 * @param queue 队列指针
 * @param data 队首数据存储位置
 * @return 操作状态
 */
QueueStatus static_queue_peek(const StaticQueue *queue, u8 *data);

/**
 * @brief 查看指定位置元素（不从队列中移除）
 * @param queue 队列指针
 * @param index 元素索引（0表示队首）
 * @param data 数据存储位置
 * @return 操作状态
 */
QueueStatus static_queue_peek_at(const StaticQueue *queue, u16 index, void *data);

#endif /* __QUEUE_H */