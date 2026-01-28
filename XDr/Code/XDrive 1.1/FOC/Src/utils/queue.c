#include "queue.h"
#include "string.h"

QueueStatus static_queue_init(StaticQueue *queue, u8 *buffer,
                              u16 capacity)
{
    if (queue == NULL || buffer == NULL || capacity == 0)
    {
        return QUEUE_ERROR;
    }

    queue->buffer = buffer;
    queue->front = 0;
    queue->rear = 0;
    queue->capacity = capacity;
    queue->count = 0;

    return QUEUE_OK;
}

void static_queue_clear(StaticQueue *queue)
{
    if (queue == NULL)
        return;

    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
}

bool static_queue_is_empty(const StaticQueue *queue)
{
    if (queue == NULL)
        return true;
    return (queue->count == 0);
}

bool static_queue_is_full(const StaticQueue *queue)
{
    if (queue == NULL)
        return true;
    return (queue->count == queue->capacity);
}

u16 static_queue_count(const StaticQueue *queue)
{
    if (queue == NULL)
        return 0;
    return queue->count;
}

u16 static_queue_remaining(const StaticQueue *queue)
{
    if (queue == NULL)
        return 0;
    return (queue->capacity - queue->count);
}

QueueStatus static_queue_enqueue(StaticQueue *queue, const u8 *data)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_ERROR;
    }

    if (static_queue_is_full(queue))
    {
        return QUEUE_FULL;
    }

    // 复制数据到队尾
    u8 *dest = queue->buffer + queue->rear;
    *dest = *data;

    // 更新队尾指针和计数
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->count++;

    return QUEUE_OK;
}

QueueStatus static_queue_dequeue(StaticQueue *queue, u8 *data)
{
    if (queue == NULL)
    {
        return QUEUE_ERROR;
    }

    if (static_queue_is_empty(queue))
    {
        return QUEUE_EMPTY;
    }

    // 如果data不为NULL，则复制队首数据
    if (data != NULL)
    {

        u8 *src = queue->buffer + queue->front;
        *data = *src;
    }

    // 更新队首指针和计数
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;

    return QUEUE_OK;
}

QueueStatus static_queue_peek(const StaticQueue *queue, u8 *data)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_ERROR;
    }

    if (static_queue_is_empty(queue))
    {
        return QUEUE_EMPTY;
    }

    u8 *src = queue->buffer + queue->front;
    *data = *src;
    return QUEUE_OK;
}

QueueStatus static_queue_peek_at(const StaticQueue *queue, u16 index, void *data)
{
    if (queue == NULL || data == NULL)
    {
        return QUEUE_ERROR;
    }

    if (static_queue_is_empty(queue))
    {
        return QUEUE_EMPTY;
    }

    if (index >= queue->count)
    {
        return QUEUE_ERROR;
    }

    u16 actual_index = (queue->front + index) % queue->capacity;
    u8 *src = queue->buffer + actual_index;
    memcpy(data, src, 1);

    return QUEUE_OK;
}
