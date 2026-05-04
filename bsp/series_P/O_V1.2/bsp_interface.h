/**
 * @file    bsp.h
 * @brief   BSP接口声明 - 板级支持包
 * @note    供app层调用，bsp实现层提供具体功能
 */

#ifndef __BSP_H
#define __BSP_H

#include <stdint.h>
#include <stdbool.h>

#ifndef __weak
#define __weak __attribute__((weak))
#endif

// 基础类型定义
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

void BSP_BL_Init(void);
void BSP_Init(void);
void BSP_Error_Handler(void);

/* ============================================
 * NVIC - 中断
 * ============================================ */
void BSP_enable_irq();
void BSP_disable_irq();
// 中断向量表偏移
void BSP_SetVectorTableOffset(u32 offset);

void BSP_Encoder_SPI_TxRxCpltCallback(void); // SPI传输完成回调函数声明
void BSP_Encoder_SPI_ErrorCallback(void);    // SPI错误回调函数声明

void BSP_FOC_ITCallback(void); // FOC定时器中断回调函数声明
/* ============================================
 * System - 系统功能
 * ============================================ */
u32 BSP_GetTick(void);
u32 BSP_GetTick_us(void);
void BSP_Delay(u32 ms);
void BSP_SystemReset(void);
/* ============================================
 * GPIO - 通用输入输出映射
 * ============================================ */
void BSP_POWER_12V_Control(bool on);
/* ============================================
 * ADC - 电流采样
 * ============================================ */
void BSP_AdcInit(void);
void BSP_AdcSampleChange(u16 compare);
void BSP_TempVbusSample(void);
void BSP_AdcGetCurrent(float *iu, float *iv, float *iw);
void BSP_AdcRecalibrateCurrent(void);
bool BSP_AdcRecalibrateDone(void);
void BSP_AdcGetVoltage(float *voltage);
void BSP_AdcGetTemp(float *temperature);

/* ============================================
 * SPI - 编码器通信
 * ============================================ */
typedef enum
{
    INTERNAL,
    EXTERNAL
} eEncoderType;

void BSP_Encoder_CS(eEncoderType type, bool level);
bool BSP_Encoder_SPI_IS_READY();
bool BSP_Encoder_SPI_TransmitReceive_DMA(u8 *tx, u8 *rx, u16 len);
void BSP_Encoder_SPI_Abort();
void BSP_Encoder_SPI_CLEAR_DMA_error_flags();

/* ============================================
 * SPI - flash通信
 * ============================================ */
void BSP_Flash_CS(bool level);
bool BSP_Flash_SPI_Transmit(u8 *tx, u16 len, u32 timeout);
bool BSP_Flash_SPI_Receive(u8 *rx, u16 len, u32 timeout);

/* ============================================
 * MCU - flash接口
 * ============================================ */
bool BSP_FLASH_ReadData(u8 *pBuffer, u32 ReadAddr, u16 NumByteToRead);
bool BSP_FLASH_WriteWord(const u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite);
bool BSP_FLASH_EraseRange(u32 start_addr, u32 end_addr);
bool BSP_Flash_EraseApp(void);
bool BSP_Flash_WriteApp(u32 addr, const u8 *data, u16 len);
bool BSP_Flash_Verify(u32 addr, const u8 *data, u16 len);
bool BSP_JumpToApp(void);
bool BSP_JumpToBootloader(const u8 *firm_version, u16 version_len);
bool BSP_GetUpgradeFlag(u8 *firm_version, u16 *version_len);
bool BSP_ClearUpgradeFlag(void);
/* ============================================
 * PWM - 电机驱动
 * ============================================ */
void BSP_PwmInit(u32 freq_hz);
void BSP_PwmSetDuty(u8 ch, float duty);
void BSP_PwmStart(void);
void BSP_PwmStop(void);

void BSP_PWM_SetCompare(u16 ticA, u16 ticB, u16 ticC);
void BSP_PWM_Enable(void);
void BSP_PWM_Disable(void);
/* ============================================
 * RGB LED - WS2812
 * ============================================ */

/* ========== 颜色结构体 ========== */
typedef struct
{
    u8 R;
    u8 G;
    u8 B;
} tRGBColor;

/* ========== 预定义颜色 ========== */
extern const tRGBColor RED;
extern const tRGBColor GREEN;
extern const tRGBColor BLUE;

extern const tRGBColor CHINA_RED;     // 中国红
extern const tRGBColor KLEIN_BLUE;    // 克莱因蓝
extern const tRGBColor MARS_GREEN;    // 马尔斯绿
extern const tRGBColor PRUSSIAN_BLUE; // 普鲁士蓝
extern const tRGBColor TIFFANY_BLUE;  // 蒂夫尼蓝
extern const tRGBColor ORANGE;        // 橙色

/* ========== LED状态枚举 ========== */

void BSP_RGBInit(void);
bool BSP_RGB_SetAllColor(tRGBColor color);
void BSP_RGB_Breathe(tRGBColor Color);

void BSP_LED_CanTogglePin(void);
void BSP_LED_EncoderTogglePin(void);
void BSP_LED_CanSetPin(bool on);
void BSP_LED_EncoderSetPin(bool on);

/* ============================================
 * CAN - 控制器局域网
 * ============================================ */
bool BSP_CanInit(u32 CAN_ID);
bool BSP_CanSetConfig(u32 CAN_ID);
bool BSP_CanSendData(u32 CAN_ID, u8 *msg, u8 len);
void BSP_CanRxCallback(bool *recv_ok, u32 *id, u8 *RxData, u32 *len); // CAN接收中断接口（供读取数据调用）

/* ============================================
 * UART - 串口
 * ============================================ */
void BSP_UART_Receive_DMA(u8 *data, u16 len);
bool BSP_UART_Transmit_DMA(u8 *data, u16 len);
void BSP_UART_RxCallback(void);
/* ============================================
 * USB - USB虚拟串口
 * ============================================ */
void BSP_USB_CS(bool level);
bool BSP_USB_CDC_Transmit_FS(u8 *data, u16 len);
bool BSP_USB_RecvByte(u8 *data, u8 *len); // USB接收中断接口（供读取数据调用）

#endif /* __BSP_H */