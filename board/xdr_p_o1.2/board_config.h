#ifndef __CONFIG_H
#define __CONFIG_H

// PWM配置
#define PWM_GET_HTIM (htim8)

// 编码器 spi接口 和 内外部cs引脚定义
#define ENCODER_SPI_CH (hspi3)
#define ENCODER_SPI SPI3
#define ENCODER_INT_CS_GPIOx GPIOA
#define ENCODER_INT_CS_GPIOx_PIN GPIO_PIN_15
#define ENCODER_EXT_CS_GPIOx GPIOA
#define ENCODER_EXT_CS_GPIOx_PIN GPIO_PIN_15

// Flash spi接口 和 cs引脚定义
#define FLASH_SPI_CH (hspi2)
#define FLASH_CS_GPIOx GPIOB
#define FLASH_CS_GPIOx_PIN GPIO_PIN_12

// RGB LED
#define RGB_PWM_GET_HTIM (htim4)
#define RGB_PWM_CHANNEL1 TIM_CHANNEL_2
#define Pixel_NUM 2 // RGB数量宏定义
#define CODE_1 (75)
#define CODE_0 (35)

#define LED_ENCODER_GPIOx GPIOD
#define LED_ENCODER_GPIOx_PIN GPIO_PIN_2
#define LED_CANrx_GPIOx GPIOB
#define LED_CANrx_GPIOx_PIN GPIO_PIN_3

// 12V POWER
#define POWER12V_GPIOx GPIOC
#define POWER12V_GPIOx_PIN GPIO_PIN_13

// USB cs pin
#define USB_CS_GPIOx GPIOA
#define USB_CS_GPIOx_PIN GPIO_PIN_8

// 串口
#define UART_CH (huart1)
#define UART_INSTANCE USART1

// CAN
#define STD_ID_MASK 0x7FF      // 标准帧11位ID掩码 [31:21]（32位模式）或 [15:5]（16位模式）
#define EXT_ID_MASK 0xFFFFFFFF // 扩展帧29位ID掩码 [31:3]

#define CAN_CH (hcan2)
#define CAN_INSTANCE CAN2

// 硬件性能
#define F_PWM 20000.0f
#define T_PWM 0.00005f
#define TIC_PWM 2099
#define T_CON 0.00005f

#define T_SAMPLE_us 7      // 采样 4-7us
#define T_DEADTIME_us 0.5f // 死区时间
#define T_NOISE_us 0.5f    // 开关噪声时间

#define RATE_CURRENT_SAMPLE 100.0f // 电流采样比率
#define RATE_VOLTAGE_SAMPLE 16     // 电压采样比率
#define MAX_CURRENT 100            // MOS管最大电流 100A
#define MAX_VOLTAGE 34             // 最大电压 34V
#define MIN_VOLTAGE 20             // 最小电压 20V
#define MAX_TEMPERATURE 80         // 最大工作温度

// ---------- 硬件版本与产品信息 ----------

#define PROD_SERIES "P"
#define FUN_V "O"
#define FIRM_V "V1.2"

// ---------- 控制分频参数 ----------
#define FREQ_HIGH_LOOP 1    // 高环分频 (1 = 每PWM周期执行)
#define FREQ_MEDIUM_LOOP 10 // 中环分频 (每FREQ_HIGH_LOOP*FREQ_MEDIUM_LOOP = 10个PWM周期)
#define FREQ_LOW_LOOP 10    // 低环分频 (每FREQ_MEDIUM_LOOP*FREQ_LOW_LOOP = 100个PWM周期)

// ---------- 数据流参数 ----------
#define T_DATA_STREAM 1     // 数据流发送间隔 (ms)
#define T_STATE_STREAM 500  // 状态包发送间隔 (ms)
#define TEMP_VBUS_TS_MS 300 // 温度/电压采样间隔 (ms)

// ---------- HFI 参数 ----------
#define HFI_INJ_VOLT_AMP 2.0f      // 高频注入电压幅值 (V)
#define HFI_INJ_FREQ_HZ 5000.0f    // 高频注入频率 (Hz)
#define HFI_PLL_BANDWIDTH_HZ 60.0f // HFI-PLL 带宽 (Hz)
#define SPEED_LPF_FACTOR 2.0f      // 速度 LPF 截止频率系数

// ========== BSP FLASH ==========

#define FLASH_START_ADDR 0x08000000U // Flash 起始地址
#define FLASH_SIZE_KB 1024           // Flash 大小 (KB)
#define FLASH_END_ADDR 0x080FFFFFU   // Flash 结束地址
#define NORMAL_MAGIC 0xFFFFFFFF      // 空数
// ========== IAP FLASH 地址规划 ==========
#define FIRMWARE_TYPE APP          // 固件类型标识，APP表示应用固件，BL表示Bootloader
#define BL_START_ADDR 0x08000000U  // Bootloader 起始地址
#define BL_SIZE_KB 32              // Bootloader 大小 (KB) 两个扇区
#define APP_START_ADDR 0x08008000U // App 起始地址
#define APP_SIZE_KB FLASH_SIZE_KB - FLAG_SIZE_KB - PARAMETER_SIZE_KB - LOG_SIZE_KB_MAX - BL_SIZE_KB

// ========== LOG FLASH 地址规划 ==========
#define LOG_SECTOR FLASH_SECTOR_9  // 日志扇区 (存日志)
#define LOG_START_ADDR 0x080A0000U // 日志起始地址
#define LOG_SIZE_KB 128            // 日志空间大小 (KB)

// ========== PARAMETER FLASH 地址规划 ==========
#define PARAMETER_SECTOR FLASH_SECTOR_10 // 参数扇区 (存参数)
#define PARAMETER_LOAD_ADDR 0x080C0000U  // 参数加载/保存地址
#define PARAMETER_SIZE_KB 128            // 参数空间大小 (KB)

// ========== 固件升级配置 ==========
#define IAP_FLAG_SECTOR FLASH_SECTOR_11 // 固件升级标志扇区 (存固件升级信息)
#define IAP_FLAG_ADDRESS 0x080E0000
#define IAP_FLAG_SIZE_KB 128

#endif