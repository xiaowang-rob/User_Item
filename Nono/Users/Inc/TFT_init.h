#ifndef TFT_init_H
#define TFT_init_H

#include "main.h"

#define USE_HORIZONTAL 0 // 设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

#define LCD_W 240 // 分辨率
#define LCD_H 240

//-----------------LCD端口定义----------------

// 数据传输用SPI HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);//发送数据
#define LCD_RES_Clr() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET) // RES 低电平复位
#define LCD_RES_Set() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET)

#define LCD_DC_Clr() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET) // DC 低电平发命令
#define LCD_DC_Set() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET)   // 高电平发数据

#define LCD_CS_Clr() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET) // CS 低电平使能
#define LCD_CS_Set() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)


void LCD_BLK(uint8_t level);                                              // 屏幕亮度调节
void LCD_WR8(uint8_t data);                                               // 片选 传输8位数据
void LCD_WR_DATA8(uint8_t dat);                                           // 写入一个字节
void LCD_WR_DATA16(uint16_t dat);                                         // 写入两个字节
void LCD_WR_REG(uint8_t dat);                                             // 写入一个指令
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); // 设置坐标函数
void LCD_Init(void);                                                      // LCD初始化
void LCD_Initshow();                                                      // 显示初始化
#endif                                                                    // TFT_init_H