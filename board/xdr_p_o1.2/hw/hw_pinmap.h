#ifndef __HW_PINMAP_H
#define __HW_PINMAP_H

// ============================================================
// hw_pinmap.h — 板级硬件映射（仅 board/<b>/hw/*.c 可见）
//
// HAL 句柄 / 引脚映射 / 定时器计数值 / 模拟前端系数 在此终结，
// 是本文件存在的唯一意义：让 usr 层永远见不到厂商库符号。
//
// 纯宏文件：不 include 任何厂商库头（tim.h/adc.h/spi.h...），
// 需要类型定义的使用者（hw/*.c）自行 include 对应库头。
//
// 来源：由原 board_config.h 中的"硬件事实"部分拆分而来；
// 板级产品/算法参数仍留在 board_config.h。
// ============================================================

// ---------- PWM（电机驱动 / FOC 节拍） ----------
#define PWM_GET_HTIM (htim8) // 电机控制定时器句柄
#define TIC_PWM 2099         // 定时器周期计数值（ARR，对应 F_PWM=20kHz）

// ---------- 编码器 SPI + CS ----------
#define ENCODER_SPI_CH (hspi3)
#define ENCODER_SPI SPI3
#define ENCODER_INT_CS_GPIOx GPIOA
#define ENCODER_INT_CS_GPIOx_PIN GPIO_PIN_15
#define ENCODER_EXT_CS_GPIOx GPIOA
#define ENCODER_EXT_CS_GPIOx_PIN GPIO_PIN_15

// ---------- Flash SPI + CS ----------
#define FLASH_SPI_CH (hspi2)
#define FLASH_CS_GPIOx GPIOB
#define FLASH_CS_GPIOx_PIN GPIO_PIN_12

// ---------- RGB (WS2812) ----------
#define RGB_PWM_GET_HTIM (htim4)
#define RGB_PWM_CHANNEL1 TIM_CHANNEL_2
#define Pixel_NUM 2  // 板上灯珠数量
#define CODE_1 (75)  // 逻辑 1 占空比 CCR 值
#define CODE_0 (35)  // 逻辑 0 占空比 CCR 值

// ---------- GPIO：LED / 电源 / USB ----------
#define LED_ENCODER_GPIOx GPIOD
#define LED_ENCODER_GPIOx_PIN GPIO_PIN_2
#define LED_CANrx_GPIOx GPIOB
#define LED_CANrx_GPIOx_PIN GPIO_PIN_3
#define POWER12V_GPIOx GPIOC
#define POWER12V_GPIOx_PIN GPIO_PIN_13
#define USB_CS_GPIOx GPIOA
#define USB_CS_GPIOx_PIN GPIO_PIN_8

// ---------- 串口 / CAN（通讯重构后置，句柄宏先收在此） ----------
#define UART_CH (huart1)
#define UART_INSTANCE USART1
#define CAN_CH (hcan2)
#define CAN_INSTANCE CAN2

// ---------- 模拟前端系数（原始值→物理量换算发生在 hw 采样资源内） ----------
#define RATE_CURRENT_SAMPLE 100.0f // 电流采样放大比率
#define RATE_VOLTAGE_SAMPLE 16     // 电压采样分压比率

#endif // __HW_PINMAP_H
