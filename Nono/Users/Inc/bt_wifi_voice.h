#include "main.h"
#include <stdbool.h>

typedef struct Ring_Buffer
{
    uint8_t *head;  // 环形缓存区开始地址
    uint8_t *tail;  // 环形缓存区结束地址
    uint8_t *write; // 环形缓存区写地址
    uint8_t *read;  // 环形缓存区读地址
    uint8_t size;   // 现有数据大小
} RingBufferTypedef;

bool RingBuffer_Init();                                  // 环形缓存区初始化
bool RingBuffer_WriteData(uint8_t *pData, uint8_t len);  // 环形缓存区写入数据
bool RingBuffer_ReadData(uint8_t *pData, uint8_t len);   // 环形缓存区读取数据
void FreeRingBuffer();                                   // 释放环形缓存区
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart); // 检测串口中断接收

bool ESP32_send_AT(char *AT);                               // 设置————AT命令
bool ESP32_query_AT(char *AT, uint8_t *data, uint8_t size); // 查询————AT命令
bool ESP32_connect_WIFI(uint8_t *ssid, uint8_t *pass);
bool ESP32_Init();
bool ESP32_SntpSetRtc();