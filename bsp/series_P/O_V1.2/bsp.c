/**
 * @file    bsp.c
 * @brief   BSP实现 - 板级支持包
 * @note    薄封装，直接调用CubeMX生成的HAL函数
 */

#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

#include "usbd_cdc_if.h"
#include "stm32f4xx_it.h"
#include "bsp_interface.h"
#include "config.h"

void SystemClock_Config(void);

void BSP_BL_Init()
{
    /* 硬件级安全锁：彻底禁止窗口看门狗，避免未配置中断就产生异常 */
    __HAL_RCC_WWDG_CLK_DISABLE();
    __HAL_RCC_WWDG_FORCE_RESET();
    __HAL_RCC_WWDG_RELEASE_RESET();

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USB_DEVICE_Init();
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void OTG_FS_IRQHandler(void)
{
    extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

void BSP_Init()
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI2_Init();
    MX_TIM4_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();
    MX_CAN2_Init();
    MX_SPI3_Init();
    MX_USART1_UART_Init();
    MX_TIM8_Init();
    MX_USB_DEVICE_Init();
}

void Error_Handler(void)
{
    BSP_Error_Handler();
    __disable_irq();
    while (1)
    {
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ============================================
 * NVIC- 中断注册 绑定
 * ============================================ */
void BSP_enable_irq()
{
    __enable_irq();
}
void BSP_disable_irq()
{
    __disable_irq();
}
// 中断向量表偏移
void BSP_SetVectorTableOffset(u32 offset)
{
    SCB->VTOR = FLASH_BASE | offset;
}

__weak void BSP_Encoder_SPI_TxRxCpltCallback(void)
{
    return;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENCODER_SPI_CH)
    {
        BSP_Encoder_SPI_TxRxCpltCallback();
    }
}

__weak void BSP_Encoder_SPI_ErrorCallback(void)
{
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &ENCODER_SPI_CH)
    {
        BSP_Encoder_SPI_ErrorCallback();
    }
}

__weak void BSP_FOC_ITCallback(void)
{
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM8)
    {
        if (TIM8->CR1 & TIM_CR1_DIR)
        {
            // ========== 上溢中断 ==========
            return;
        }
        else
        {
            // ========== 下溢中断 ==========
            BSP_FOC_ITCallback();
            return;
        }
    }
}
/* ============================================
 * System - 系统功能
 * ============================================ */
u32 BSP_GetTick(void)
{
    return HAL_GetTick();
}

u32 BSP_GetTick_us(void)
{
    /* 计算微秒级时间戳 */
    u32 tick_ms = BSP_GetTick();                                                        // 获取当前毫秒级系统时间
    u32 tick_us = (tick_ms * 1000) + (DWT->CYCCNT / (HAL_RCC_GetHCLKFreq() / 1000000)); // 转换为微秒
    return tick_us;
}

void BSP_Delay(u32 ms)
{
    HAL_Delay(ms);
}

void BSP_SystemReset(void)
{
    NVIC_SystemReset();
}
/* ============================================
 * GPIO - 通用输入输出映射
 * ============================================ */
void BSP_POWER_12V_Control(bool on)
{
    if (on)
    {
        HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(POWER12V_GPIOx, POWER12V_GPIOx_PIN, GPIO_PIN_RESET);
    }
}

/* ============================================
 * SPI - 编码器接口
 * ============================================ */
// 内部接口CS
void BSP_Encoder_CS(eEncoderType type, bool level)
{
    if (type == INTERNAL)
    {
        if (level)
        {
            HAL_GPIO_WritePin(ENCODER_INT_CS_GPIOx, ENCODER_INT_CS_GPIOx_PIN, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(ENCODER_INT_CS_GPIOx, ENCODER_INT_CS_GPIOx_PIN, GPIO_PIN_RESET);
        }
    }
    else // EXTERNAL
    {
        if (level)
        {
            HAL_GPIO_WritePin(ENCODER_EXT_CS_GPIOx, ENCODER_EXT_CS_GPIOx_PIN, GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(ENCODER_EXT_CS_GPIOx, ENCODER_EXT_CS_GPIOx_PIN, GPIO_PIN_RESET);
        }
    }
}

bool BSP_Encoder_SPI_IS_READY()
{
    return HAL_SPI_GetState(&ENCODER_SPI_CH) == HAL_SPI_STATE_READY;
}

bool BSP_Encoder_SPI_TransmitReceive_DMA(u8 *tx, u8 *rx, u16 len)
{
    return HAL_SPI_TransmitReceive_DMA(&ENCODER_SPI_CH, tx, rx, len) == HAL_OK;
}

void BSP_Encoder_SPI_Abort()
{
    HAL_SPI_Abort(&ENCODER_SPI_CH);
}

void BSP_Encoder_SPI_CLEAR_DMA_error_flags()
{
    __HAL_SPI_CLEAR_OVRFLAG(&ENCODER_SPI_CH);
    __HAL_SPI_CLEAR_FREFLAG(&ENCODER_SPI_CH);
}

/* ============================================
 * SPI - flash接口
 * ============================================ */
void BSP_Flash_CS(bool level)
{
    if (level)
    {
        HAL_GPIO_WritePin(FLASH_CS_GPIOx, FLASH_CS_GPIOx_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(FLASH_CS_GPIOx, FLASH_CS_GPIOx_PIN, GPIO_PIN_RESET);
    }
}
bool BSP_Flash_SPI_Transmit(u8 *tx, u16 len, u32 timeout)
{
    return HAL_SPI_Transmit(&FLASH_SPI_CH, tx, len, timeout) == HAL_OK;
}
bool BSP_Flash_SPI_Receive(u8 *rx, u16 len, u32 timeout)
{
    return HAL_SPI_Receive(&FLASH_SPI_CH, rx, len, timeout) == HAL_OK;
}
/* ============================================
 * PWM - 电机驱动
 * ============================================ */
void BSP_PWM_SetCompare(u16 ticA, u16 ticB, u16 ticC)
{
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, ticA);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, ticB);
    __HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_3, ticC);
}
void BSP_PWM_Enable(void)
{
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);

    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim8, TIM_CHANNEL_3);
}
void BSP_PWM_Disable(void)
{
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim8, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim8, TIM_CHANNEL_3);
}
/* ============================================
 * CAN - 控制器局域网
 * ============================================ */
bool BSP_CanInit(u32 CAN_ID)
{
    /* 配置CAN过滤器 */
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 14;                     // CAN2专用过滤器组（14-27）
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // 掩码模式，精确匹配单个ID
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; // 32位过滤器尺度
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    /* 计算标准帧ID在过滤器寄存器中的值：STID[10:0]位于[31:21]，IDE=0（标准帧），RTR=0（数据帧） */
    u32 std_id = CAN_ID & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00;
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    /* 设置掩码：匹配11位ID + IDE位(0) + RTR位(0)，其余位忽略 */
    u32 mask_reg = (STD_ID_MASK << 21) | 0x06;
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xFFFF;

    /* 应用过滤器配置并更新设备状态 */
    if (HAL_CAN_ConfigFilter(&CAN_CH, &sFilterConfig) != HAL_OK)
    {
        return false;
    }

    /* 启动CAN外设 */
    if (HAL_CAN_Start(&CAN_CH) != HAL_OK)
    {
        return false;
    }

    /* 使能FIFO0接收中断 */
    if (HAL_CAN_ActivateNotification(&CAN_CH, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        return false;
    }
    return true;
}
bool BSP_CanSetConfig(u32 CAN_ID)
{
    /* 停止CAN外设以安全重配置 */
    HAL_CAN_Stop(&CAN_CH);

    /* 配置CAN过滤器（参数同初始化函数） */
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 14;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

    u32 std_id = CAN_ID & STD_ID_MASK;
    u32 id_reg = (std_id << 21) | 0x00;
    sFilterConfig.FilterIdHigh = (id_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterIdLow = id_reg & 0xFFFF;

    u32 mask_reg = (STD_ID_MASK << 21) | 0x06;
    sFilterConfig.FilterMaskIdHigh = (mask_reg >> 16) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = mask_reg & 0xFFFF;

    /* 应用新过滤器配置 */
    if (HAL_CAN_ConfigFilter(&CAN_CH, &sFilterConfig) != HAL_OK)
    {
        return false;
    }

    /* 重启CAN外设 */
    if (HAL_CAN_Start(&CAN_CH) != HAL_OK)
    {
        return false;
    }

    /* 重新使能接收中断 */
    if (HAL_CAN_ActivateNotification(&CAN_CH, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        return false;
    }
    return true;
}
bool BSP_CanSendData(u32 CAN_ID, u8 *msg, u8 len)
{
    u8 i = 0;
    u8 message[8];                    // 本地发送缓冲区（CAN数据帧最大8字节）
    u32 TxMailbox;                    // 实际使用的发送邮箱编号
    CAN_TxHeaderTypeDef CAN_TxHeader; // CAN发送帧头配置

    /* 配置发送帧头参数 */
    CAN_TxHeader.StdId = CAN_ID;               // 标准帧ID（11位）
    CAN_TxHeader.ExtId = CAN_ID;               // 扩展帧ID（本实现未使用）
    CAN_TxHeader.IDE = CAN_ID_STD;             // 标准帧格式
    CAN_TxHeader.RTR = CAN_RTR_DATA;           // 数据帧（非远程帧）
    CAN_TxHeader.DLC = len;                    // 数据长度（0~8字节）
    CAN_TxHeader.TransmitGlobalTime = DISABLE; // 禁用时间戳

    /* 复制用户数据到本地缓冲区 */
    for (i = 0; i < len; i++)
    {
        message[i] = msg[i];
    }

    /* 将消息加入发送邮箱 */
    if (HAL_CAN_AddTxMessage(&CAN_CH, &CAN_TxHeader, message, &TxMailbox) != HAL_OK)
    {
        return false;
    }

    /* 阻塞等待发送完成（3个邮箱均空闲表示发送结束） */
    while (HAL_CAN_GetTxMailboxesFreeLevel(&CAN_CH) < 3)
    {
        // 空循环等待
    }

    return true;
}

__weak void BSP_CanRxCallback(bool *recv_ok, u32 *id, u8 *RxData, u32 *len) // CAN接收中断接口（供读取数据调用）
{
    return;
}

void Bsp_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    u8 RxData[8];                     // 接收数据缓冲区
    CAN_RxHeaderTypeDef CAN_RxHeader; // 接收帧头信息
    bool recv_ok = false;

    /* 从FIFO0读取消息 */
    recv_ok = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CAN_RxHeader, RxData) == HAL_OK;
    BSP_CanRxCallback(&recv_ok, &CAN_RxHeader.StdId, RxData, &CAN_RxHeader.DLC);
}

/* ============================================
 * UART - 串口
 * ============================================ */
void BSP_UART_Receive_DMA(uint8_t *data, u16 len)
{
    HAL_UART_Receive_DMA(&UART_CH, data, len);
}
bool BSP_UART_Transmit_DMA(uint8_t *data, u16 len)
{
    return HAL_UART_Transmit_DMA(&UART_CH, data, len) == HAL_OK;
}

__weak void BSP_UART_RxCallback()
{
    return;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART_INSTANCE)
    {
        BSP_UART_RxCallback();
    }
}

/* ============================================
 * USB - USB虚拟串口
 * ============================================ */
void BSP_USB_CS(bool level)
{
    if (level)
    {
        HAL_GPIO_WritePin(USB_CS_GPIOx, USB_CS_GPIOx_PIN, GPIO_PIN_SET);
    }
    else
    {
        HAL_GPIO_WritePin(USB_CS_GPIOx, USB_CS_GPIOx_PIN, GPIO_PIN_RESET);
    }
}
bool BSP_USB_CDC_Transmit_FS(uint8_t *data, uint16_t len)
{
    return CDC_Transmit_FS(data, len) == USBD_OK;
}

__weak bool BSP_USB_RecvByte(u8 *data, u8 *len)
{
    return false;
}
void USB_RecvByte(uint8_t *Buf, uint32_t *Len)
{
    BSP_USB_RecvByte(Buf, (u8 *)Len);
}