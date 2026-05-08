#ifndef __CONFIG_H
#define __CONFIG_H

// PWM配置
#define PWM_GET_HTIM (htim8)

// 编码器 spi接口 和 内外部cs引脚定义
#define ENCODER_SPI_CH (hspi3)
#define ENCODER_INT_CS_GPIOx GPIOA
#define ENCODER_INT_CS_GPIOx_PIN GPIO_PIN_15
#define ENCODER_EXT_CS_GPIOx GPIOA
#define ENCODER_EXT_CS_GPIOx_PIN GPIO_PIN_15

// Flash spi接口 和 cs引脚定义
#define FLASH_SPI_CH (hspi2)
#define FLASH_CS_GPIOx GPIOB
#define FLASH_CS_GPIOx_PIN GPIO_PIN_12

/*RGB LED*/
#define RGB_PWM_GET_HTIM (htim4)
#define RGB_PWM_CHANNEL1 TIM_CHANNEL_2
#define Pixel_NUM 2 // RGB数量宏定义
#define CODE_1 (75)
#define CODE_0 (35)

#define LED_ENCODER_GPIOx GPIOD
#define LED_ENCODER_GPIOx_PIN GPIO_PIN_2
#define LED_CANrx_GPIOx GPIOB
#define LED_CANrx_GPIOx_PIN GPIO_PIN_3

/*12V POWER*/
#define POWER12V_GPIOx GPIOC
#define POWER12V_GPIOx_PIN GPIO_PIN_13

/*USB cs pin*/
#define USB_CS_GPIOx GPIOA
#define USB_CS_GPIOx_PIN GPIO_PIN_8

/*串口*/
#define UART_CH (huart1)
#define UART_INSTANCE USART1

/*CAN*/
#define STD_ID_MASK 0x7FF      // 标准帧11位ID掩码 [31:21]（32位模式）或 [15:5]（16位模式）
#define EXT_ID_MASK 0xFFFFFFFF // 扩展帧29位ID掩码 [31:3]

#define CAN_CH (hcan2)
#define CAN_INSTANCE CAN2

// 硬件性能
#define F_PWM 20000.0f
#define T_PWM 0.00005f
#define TIC_PWM 2099
#define T_CON 0.00005f

#define T_SAMPLE_us 7   // 采样 4-7us
#define T_DEATH_us 0.5f // 死区时间
#define T_NOISE_us 0.5f // 开关噪声时间

#define MED_FILTER_SIZE 5          // 中值滤波器大小，必须为奇数
#define RATE_CURRENT_SAMPLE 100.0f // 电流采样比率
#define RATE_VOLTAGE_SAMPLE 16     // 电压采样比率
#define MAX_CURRENT 100            // MOS管最大电流 100A
#define MAX_VOLTAGE 34             // 最大电压 34V
#define MIN_VOLTAGE 20             // 最小电压 20V
#define MAX_TEMPERATURE 80         // 最大工作温度

/* ---------- 硬件版本与产品信息 ---------- */

#define PROD_SERIES "P"
#define FUN_V "O"
#define FIRM_V "V1.2"

/* ========== Flash 地址规划 ========== */
#define FIRMWARE_TYPE APP // 固件类型标识，APP表示应用固件，BL表示Bootloader

#define BL_START_ADDR 0x08000000U     // Bootloader 起始地址
#define BL_SIZE_KB 32                 // Bootloader 大小 (KB)
#define APP_START_ADDR 0x08008000U    // App 起始地址
#define FLASH_END_ADDR 0x080FFFFFU    // F405 1MB Flash 结束地址
#define CONFIG_SECTOR FLASH_SECTOR_11 // 配置扇区 (存升级标志)，不擦除

/* ========== 固件升级配置 ========== */
#define FLAG_ADDRESS 0x080E0000
#define NORMAL_MAGIC 0xFFFFFFFF

#endif