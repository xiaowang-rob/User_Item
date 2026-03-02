/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "led.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS; /* 来自 usb_device.c */

/* 升级命令全局变量 (usbd_cdc_if.c 中定义) */
extern volatile uint8_t g_upgrade_cmd;
extern volatile uint32_t g_upgrade_addr;
extern volatile uint16_t g_upgrade_len;
extern volatile uint8_t g_upgrade_data[256];
extern volatile uint8_t g_cmd_received;

/* 升级状态 */
volatile uint8_t g_in_upgrade_mode = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  LED_Init();
  LED_SetState(LED_IDLE);
  /* 检查升级标志，有则进入升级模式 */
  if (Check_Upgrade_Flag())
  {
    Clear_Upgrade_Flag(); /* 清除标志 */
    g_in_upgrade_mode = 1;
    LED_SetState(LED_IDLE);
  }

  if (g_in_upgrade_mode)
  {
    /* 升级模式主循环 */
    while (1)
    {
      LED_Process(); /* 非阻塞更新 LED */

      if (g_cmd_received)
      {
        g_cmd_received = 0;

        /* 根据命令类型设置对应 LED 状态 */
        switch (g_upgrade_cmd)
        {
        case 0x01: /* 进入升级确认 */
          LED_SetState(LED_IDLE);
          break;

        case 0x02: /* 擦除 Flash */
          LED_SetState(LED_ERASING);
          break;

        case 0x03: /* 写入 Flash */
          LED_SetState(LED_WRITING);
          break;

        case 0x04: /* 校验 Flash */
          LED_SetState(LED_VERIFYING);
          break;

        case 0x05: /* 跳转 App */
          LED_SetState(LED_SUCCESS);
          break;

        default:
          LED_SetState(LED_ERROR);
          break;
        }

        uint8_t result = Process_Upgrade_Cmd(g_upgrade_cmd, g_upgrade_addr,
                                             (uint8_t *)g_upgrade_data, g_upgrade_len);

        /* 如果是跳转命令，显示结果后执行 */
        if (g_upgrade_cmd == 0x05)
        {
          if (result)
          {
            LED_SetState(LED_SUCCESS);
            HAL_Delay(300);
            JumpToApp();
          }
          else
          {
            /* 跳转失败 - 全灭或同步快闪表示严重错误 */
            LED_SetState(LED_ERROR);
          }
        }
        else if (!result)
        {
          /* 其他命令失败 */
          LED_SetState(LED_ERROR);
        }
      }
      HAL_Delay(1);
    }
  }

  /* 无升级标志，直接跳转 App */
  JumpToApp();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
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

/* USER CODE BEGIN 4 */

/**
 * @brief 检查升级标志
 * @retval 1: 需要升级，0: 正常运行
 */
uint8_t Check_Upgrade_Flag(void)
{
  return (*(__IO uint32_t *)FLAG_ADDRESS == UPGRADE_MAGIC) ? 1 : 0;
}

/**
 * @brief 设置升级标志
 */
void Set_Upgrade_Flag(void)
{
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef erase_init;
  uint32_t sector_error;

  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  /* 修改为 Sector 11 */
  erase_init.Sector = FLASH_SECTOR_11;
  erase_init.NbSectors = 1;

  if (HAL_FLASHEx_Erase(&erase_init, &sector_error) == HAL_OK)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLAG_ADDRESS, UPGRADE_MAGIC);
  }

  HAL_FLASH_Lock();
}

/**
 * @brief 清除升级标志
 */
void Clear_Upgrade_Flag(void)
{
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef erase_init;
  uint32_t sector_error;

  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  /* 修改为 Sector 11 */
  erase_init.Sector = FLASH_SECTOR_11;
  erase_init.NbSectors = 1;

  if (HAL_FLASHEx_Erase(&erase_init, &sector_error) == HAL_OK)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLAG_ADDRESS, NORMAL_MAGIC);
  }

  HAL_FLASH_Lock();
}

/**
 * @brief 擦除 App 区 Flash (Sector 1-10)
 * @retval 1: 成功，0: 失败
 */
uint8_t Flash_Erase_App(void)
{
  FLASH_EraseInitTypeDef erase_init;
  uint32_t sector_error;

  HAL_FLASH_Unlock();

  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase_init.Sector = FLASH_SECTOR_1; /* App 起始扇区 */
  erase_init.NbSectors = 10;          /* Sector 1-10 */

  uint8_t result = (HAL_FLASHEx_Erase(&erase_init, &sector_error) == HAL_OK) ? 1 : 0;

  HAL_FLASH_Lock();
  return result;
}

/**
 * @brief 写入 Flash
 * @param addr: 目标地址
 * @param data: 数据指针
 * @param len: 数据长度 (字节)
 * @retval 1: 成功，0: 失败
 */
uint8_t Flash_Write(uint32_t addr, uint8_t *data, uint16_t len)
{
  HAL_StatusTypeDef status = HAL_OK;
  HAL_FLASH_Unlock();

  for (uint16_t i = 0; i < len && status == HAL_OK; i += 4)
  {
    uint32_t word = 0;
    word = data[i];
    if (i + 1 < len)
      word |= (uint32_t)data[i + 1] << 8;
    if (i + 2 < len)
      word |= (uint32_t)data[i + 2] << 16;
    if (i + 3 < len)
      word |= (uint32_t)data[i + 3] << 24;

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, word);
    addr += 4;
  }

  HAL_FLASH_Lock();
  return (status == HAL_OK) ? 1 : 0;
}

/**
 * @brief 校验 Flash
 * @retval 1: 成功，0: 失败
 */
uint8_t Flash_Verify(uint32_t addr, uint8_t *data, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    if (*(uint8_t *)(addr + i) != data[i])
    {
      return 0;
    }
  }
  return 1;
}
/**
 * @brief 跳转到 App
 */
void JumpToApp(void)
{
  uint32_t app_stack = *(__IO uint32_t *)0x08004000;
  uint32_t app_reset = *(__IO uint32_t *)0x08004004;

  // 简单检查栈地址是否合理
  if ((app_stack & 0x2FFE0000) == 0x20000000)
  {
    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __set_MSP(app_stack);

    ((void (*)(void))app_reset)();
  }
  else
  {
    LED_SetState(LED_ERROR);
    // 跳转失败，死循环
    while (1)
    {
      LED_Process();
      HAL_Delay(1);
    }
  }
}
/**
 * @brief 处理升级命令
 * @retval 1: 成功，0: 失败
 */
uint8_t Process_Upgrade_Cmd(uint8_t cmd, uint32_t addr, uint8_t *data, uint16_t len)
{
  uint8_t response[3] = {0x3A, cmd, 0x00};
  uint8_t result = 1;

  switch (cmd)
  {
  case 0x01: /* 进入升级 (确认) */
    result = 1;
    break;

  case 0x02: /* 擦除 Flash */
    result = Flash_Erase_App();
    break;

  case 0x03: /* 写入 Flash */
    /* [修改点 7] 地址对齐检查 */
    if (addr % 4 != 0)
    {
      result = 0; /* 地址必须 4 字节对齐 */
    }
    else
    {
      result = Flash_Write(addr, data, len);
    }
    break;

  case 0x04: /* 校验 Flash */
    result = Flash_Verify(addr, data, len);
    break;

  case 0x05: /* 跳转 App */
    result = 1;
    break;

  default:
    result = 0;
    break;
  }

  response[2] = result ? 0x00 : 0x01;

  /* [修改点 4] USB 发送状态检查与重试 */
  uint8_t retry = 0;
  while (CDC_Transmit_FS(response, 3) != USBD_OK && retry < 50)
  {
    HAL_Delay(2);
    retry++;
  }

  return result;
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
