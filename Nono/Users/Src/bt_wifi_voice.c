/*
串口 USART2
波特率 115200
功能 和esp8266通讯 获取是获取时间信息（等）
协议

*/
#include "bt_wifi_voice.h"
#include "usart.h"
#include <string.h>
#include "TFT.h"
#include "rtc.h"
#include "cmsis_os.h"


#define RX_Buffer_MAX 256
RingBufferTypedef RX_buffer;
uint8_t BUFFER[RX_Buffer_MAX]; // 存储空间

uint8_t Usart_buffer[RX_Buffer_MAX]; // 串口数据缓冲区
/*标志位*/
uint8_t DATA_flag = 0;  // 数据传输标志位 0--不是所需数据 1--接收请求数据
uint8_t TxData_len = 0; // 传输数据长度
uint8_t RxData_len = 0; // 接收数据长度
uint8_t OK_flag = 0;    // AT指令正确发送标志
/*获取的SNTP时间数据*/
RTC_DateTypeDef GETDate;
RTC_TimeTypeDef GETTime;

uint8_t init_number=3;//ESP32初始化次数
/*
回调函数
对三个串口发送返回数据集中处理
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /*(Usart2 ESP32回调)*/
  if (huart == &huart2)
  {

    if (DATA_flag == 0)
    {
      if ((Usart_buffer[TxData_len + 2] == 'O' && Usart_buffer[TxData_len + 3] == 'K'))
      {
        OK_flag = 1;
      }
      else
      {
        HAL_UART_Receive_IT(&huart2, (uint8_t *)Usart_buffer, TxData_len + 6);
      }
    }
    else
    {
      if ((Usart_buffer[RxData_len - 4] == 'O') && (Usart_buffer[RxData_len - 3] == 'K'))
      {
        OK_flag = 1;
      }
      else
      {
        HAL_UART_Receive_IT(&huart2, (uint8_t *)Usart_buffer, RxData_len);
      }
    }
  }
}

/*环形缓存区定义*/
bool RingBuffer_Init() // 环形缓存区初始化
{
  /*静态空间*/
  memset(BUFFER, 0, RX_Buffer_MAX);

  RX_buffer.head = BUFFER;
  RX_buffer.tail = &BUFFER[RX_Buffer_MAX - 1];
  RX_buffer.read = BUFFER;
  RX_buffer.write = BUFFER;
  RX_buffer.size = 0;
  return true;
}
bool RingBuffer_WriteData(uint8_t *pData, uint8_t len) // 环形缓存区写入数据
{
  if ((RX_Buffer_MAX - RX_buffer.size) < len)
    return false;

  if (RX_buffer.tail - RX_buffer.write <= len)
  {
    uint8_t taillen = RX_buffer.tail - RX_buffer.write;
    memcpy(RX_buffer.write, pData, taillen);
    RX_buffer.write = RX_buffer.head;
    memcpy(RX_buffer.write, pData + taillen, len - taillen);
    RX_buffer.write += (len - taillen);
  }
  else
  {
    memcpy(RX_buffer.write, pData, len);
    RX_buffer.write += len;
  }
  RX_buffer.size += len;
  return true;
}
bool RingBuffer_ReadData(uint8_t *pData, uint8_t len) // 环形缓存区读取数据
{
  if (RX_buffer.size < len)
    return false;

  if (RX_buffer.tail - RX_buffer.read < len)
  {
    uint8_t taillen = RX_buffer.tail - RX_buffer.read;
    memcpy(pData, RX_buffer.write, taillen);
    RX_buffer.write = RX_buffer.head;
    memcpy(pData + taillen, RX_buffer.write, len - taillen);
    RX_buffer.write += (len - taillen);
  }
  else
  {
    memcpy(pData, RX_buffer.read, len);
    RX_buffer.read += len;
    if (RX_buffer.read == RX_buffer.tail)
      RX_buffer.read = RX_buffer.head;
  }
  RX_buffer.size -= len;

  return true;
}
void FreeRingBuffer() // 释放环形缓存区
{
  RingBuffer_Init();
}
bool ESP32_send_AT(char *AT)
{
  int i = 0;
  TxData_len = strlen(AT);
  HAL_UART_Transmit_IT(&huart2, (uint8_t *)AT, TxData_len);
  HAL_UART_Receive_IT(&huart2, Usart_buffer, TxData_len + 6);
  while (!OK_flag)
  {
    osDelay(1);
    if (i++ > 1000)
      return false;
  }
  OK_flag = 0;
  memset(Usart_buffer, 0, 256);
  return true;
}
bool ESP32_query_AT(char *AT, uint8_t *data, uint8_t size)
{
  int i = 0;
  DATA_flag = 1;
  TxData_len = strlen(AT);
  RxData_len = size;
  HAL_UART_Transmit_IT(&huart2, (uint8_t *)AT, TxData_len);
  HAL_UART_Receive_IT(&huart2, Usart_buffer, size);
  while (!OK_flag)
  {
    osDelay(1);
    if (i++ > 1000)
      return false;
  }
  OK_flag = 0;
  DATA_flag = 0;
  memcpy(data, Usart_buffer + TxData_len, RxData_len - TxData_len);
  //   RingBuffer_WriteData(Usart_buffer + TxData_len, RxData_len - TxData_len);
  //   RingBuffer_ReadData(data, RxData_len - TxData_len);
  memset(Usart_buffer, 0, 255);
  return true;
}
bool ESP32_Init()
{
  for (int i = 0; i < init_number;init_number--)
  {
    if (ESP32_send_AT("AT\r\n"))
    {
      while ((ESP32_send_AT("AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n")) == 0)
        osDelay(1);
      osDelay(1000);
      return true;
    }
    else
    {
      if (init_number == 1)
      {
        ESP32_send_AT("AT+RST\r\n");
        osDelay(1000);
        ESP32_Init();
				init_number--;
      }
      osDelay(1000);
    }
  }
  return false;
}

bool ESP32_SntpSetRtc()
{
  uint8_t SNTP_Time[40];
  ESP32_query_AT("AT+CIPSNTPTIME?\r\n", SNTP_Time, 60);
  uint8_t month = 0;
  uint8_t weekday = 0;
  switch (SNTP_Time[17])
  {
  case 'J':
  {
    if (SNTP_Time[18] == 'a')
      month = 1;
    else if (SNTP_Time[18] == 'u')
    {
      if (SNTP_Time[19] == 'n')
        month = 6;
      else
        month = 7;
    }
  }
  break;
  case 'F':
    month = 2;
    break;
  case 'M':
  {
    if (SNTP_Time[19] == 'r')
      month = 3;
    else
      month = 5;
  }
  break;
  case 'A':
  {
    if (SNTP_Time[18] == 'p')
      month = 4;
    else
      month = 8;
  }
  break;
  case 'S':
    month = 9;
    break;
  case 'O':
    month = 10;
    break;
  case 'N':
    month = 11;
    break;
  case 'D':
    month = 12;
    break;
  default:
    break;
  }
  /*星期*/
  switch (SNTP_Time[13])
  {
  case 'M':
    weekday = 1;
    break;
  case 'T':
  {
    if (SNTP_Time[14] == 'u')
      weekday = 2;
    else
      weekday = 4;
  }
  break;
  case 'W':
    weekday = 3;
    break;
  case 'F':
    weekday = 5;
    break;

  case 'S':
  {
    if (SNTP_Time[14] == 'a')
      weekday = 6;
    else
      weekday = 7;
  }
  break;
  default:
    break;
  }
  GETDate.Year = (SNTP_Time[36] - 48) + (SNTP_Time[35] - 48) * 10;
  GETDate.Month = month;
  GETDate.Date = (SNTP_Time[22] - 48) + (SNTP_Time[21] - 48) * 10;
  GETDate.WeekDay = weekday;
  GETTime.Seconds = (uint8_t)((SNTP_Time[31] - 48) + (SNTP_Time[30] - 48) * 10 + 2);
  GETTime.Minutes = (uint8_t)((SNTP_Time[28] - 48) + (SNTP_Time[27] - 48) * 10);
  GETTime.Hours = (uint8_t)((SNTP_Time[25] - 48) + (SNTP_Time[24] - 48) * 10);
  HAL_RTC_SetTime(&hrtc, &GETTime, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &GETDate, RTC_FORMAT_BIN);
  return true;
}
