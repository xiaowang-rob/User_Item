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
#include "stdbool.h"
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

volatile uint32_t g_firmware_total_size = 0; // 固件总大小
volatile uint8_t g_erase_sectors_start = 0;  // 起始扇区号
volatile uint8_t g_erase_sectors_count = 0;  // 擦除扇区数量

volatile uint32_t upgrade_addr;

volatile bool g_in_upgrade_mode = false;
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
        case CMD_IAP_ENTER: /* 进入升级确认 */
          LED_SetState(LED_IDLE);
          break;

        case CMD_IAP_ERASE_FLASH:  /* 擦除 Flash */
        case CMD_IAP_WRITE_FLASH:  /* 写入 Flash */
        case CMD_IAP_VERIFY_FLASH: /* 校验 Flash */
          LED_SetState(LED_WRITING);
          break;

        case CMD_IAP_EXIT: /* 跳转 App */
          LED_SetState(LED_SUCCESS);
          break;

        default:
          LED_SetState(LED_ERROR);
          break;
        }
        /* === 开启临界区保护 === */
        __disable_irq(); // 1. 关中断

        /* 2. 一次性将 volatile 全局变量拷贝到局部变量 */
        uint8_t cmd = g_upgrade_cmd;
        uint16_t len = g_upgrade_len;
        uint8_t data_buf[256];

        if (len > 0 && len <= 256)
        {
          memcpy(data_buf, (void *)g_upgrade_data, len);
        }

        g_cmd_received = 0; // 3. 清除标志

        __enable_irq(); // 4. 开中断
                        /* === 临界区结束 === */
        uint8_t result = Process_Upgrade_Cmd(cmd, data_buf, len);

        /* 如果是跳转命令，显示结果后执行 */
        if (g_upgrade_cmd == CMD_IAP_EXIT)
        {
          if (result)
          {
            LED_SetState(LED_SUCCESS);
            for (int i = 0; i < 200; i++)
            {
              HAL_Delay(10);
              LED_Process();
            }
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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
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
 * @brief 根据 Flash 地址计算扇区号
 * @param addr: Flash 地址
 * @retval 扇区号 (0-11)，失败返回 0xFF
 */
uint8_t Flash_GetSectorFromAddr(uint32_t addr)
{
  // STM32F405 Flash 扇区映射表
  static const uint32_t sector_start_addr[] = {
      0x08000000, // Sector 0:  16KB
      0x08004000, // Sector 1:  16KB
      0x08008000, // Sector 2:  16KB
      0x0800C000, // Sector 3:  16KB
      0x08010000, // Sector 4:  64KB
      0x08020000, // Sector 5:  128KB
      0x08040000, // Sector 6:  128KB
      0x08060000, // Sector 7:  128KB
      0x08080000, // Sector 8:  128KB
      0x080A0000, // Sector 9:  128KB
      0x080C0000, // Sector 10: 128KB
      0x080E0000, // Sector 11: 128KB
  };

  for (uint8_t i = 0; i < 12; i++)
  {
    if (addr >= sector_start_addr[i])
    {
      if (i == 11)
        return 11; // 最后一个扇区
      if (addr < sector_start_addr[i + 1])
        return i;
    }
  }
  return 0xFF; // 无效地址
}
/**
 * @brief 根据起始地址和固件大小计算需要擦除的扇区
 * @param start_addr: 起始地址
 * @param total_size: 固件总大小
 * @retval 1: 成功，0: 失败
 */
uint8_t Flash_CalcEraseSectors(uint32_t start_addr, uint32_t total_size)
{
  if (total_size == 0)
    return 0; // 固件大小为 0，无效

  uint32_t end_addr = start_addr + total_size;

  // 获取起始扇区
  uint8_t start_sector = Flash_GetSectorFromAddr(start_addr);
  if (start_sector == 0xFF)
    return 0;

  // 获取结束扇区 (end_addr-1 是因为地址是开区间)
  uint8_t end_sector = Flash_GetSectorFromAddr(end_addr - 1);
  if (end_sector == 0xFF)
    return 0;

  // 安全检查 1: 不能擦除 Sector 0 (Bootloader 所在)
  if (start_sector == 0)
    return 0;

  // 安全检查 2: 不能擦除 CONFIG_SECTOR (存升级标志)
  if (end_sector >= CONFIG_SECTOR)
  {
    end_sector = CONFIG_SECTOR - 1;
  }

  // 计算扇区数量
  g_erase_sectors_start = start_sector;
  g_erase_sectors_count = end_sector - start_sector + 1;

  return 1;
}
/**
 * @brief 擦除 App 区 Flash (使用动态计算的扇区)
 * @retval 1: 成功，0: 失败
 */
uint8_t Flash_Erase_App(void)
{
  // 检查是否已计算擦除范围
  if (g_erase_sectors_count == 0)
    return 0;

  FLASH_EraseInitTypeDef erase_init;
  uint32_t sector_error;

  HAL_FLASH_Unlock();

  erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
  erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  erase_init.Sector = g_erase_sectors_start;    // 动态起始扇区
  erase_init.NbSectors = g_erase_sectors_count; // 动态扇区数量

  uint8_t result = (HAL_FLASHEx_Erase(&erase_init, &sector_error) == HAL_OK) ? 1 : 0;

  HAL_FLASH_Lock();
  return result;
}
/**
 * @brief 写入 Flash (支持非 4 字节对齐)
 * @param addr: 目标地址
 * @param data: 数据指针
 * @param len: 数据长度 (字节)
 * @retval 1: 成功，0: 失败
 */
uint8_t Flash_Write(uint32_t addr, uint8_t *data, uint16_t len)
{
  HAL_StatusTypeDef status = HAL_OK;
  HAL_FLASH_Unlock();

  uint16_t i = 0;

  /* 1. 先按 4 字节 (Word) 写入，效率高 */
  for (; i + 3 < len && status == HAL_OK; i += 4)
  {
    uint32_t word = data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24);
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, word);
    addr += 4;
  }

  /* 2. 处理剩余不足的 4 字节 (1~3 字节)，按字节 (Byte) 写入 */
  while (i < len && status == HAL_OK)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr, data[i]);
    addr++;
    i++;
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
uint8_t Process_Upgrade_Cmd(uint8_t cmd, uint8_t *data, uint16_t len)
{
  uint8_t response[6] = {0x3A, cmd, 0x01, 0x00, 0x00, 0x0D};
  uint8_t result = 1;

  switch (cmd)
  {
  case CMD_IAP_ENTER: /* 进入升级 (确认) */
    if (len >= 4)
    {
      // 解析 4 字节小端序固件大小
      g_firmware_total_size = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);

      // 计算需要擦除的扇区范围
      if (Flash_CalcEraseSectors(APP_START_ADDR, g_firmware_total_size))
      {
        result = 1;
      }
      else
      {
        result = 0; // 扇区计算失败
      }
    }
    else
    {
      result = 0; // 数据长度不足 4 字节
    }
    break;
  case CMD_IAP_ERASE_FLASH: /* 擦除 Flash */
    //result = Flash_Erase_App();
		result=1;
    break;

  case CMD_IAP_WRITE_FLASH: /* 写入 Flash */

    upgrade_addr = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
                   ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    if (upgrade_addr < 0x08004000 || upgrade_addr >= 0x080FFFFF)
    {
      result = 0; // 非法地址，直接丢弃
      break;
    }
    g_upgrade_len = len - 4; /* 减去地址(4B) */

    /* [修改点 7] 地址对齐检查 */
    if (upgrade_addr % 4 != 0)
    {
      result = 0; /* 地址必须 4 字节对齐 */
    }
    else
    {
      //result = Flash_Write(upgrade_addr, &data[4], g_upgrade_len);
			result=1;
    }
    break;

  case CMD_IAP_VERIFY_FLASH: /* 校验 Flash */
    upgrade_addr = (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
                   ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    g_upgrade_len = len - 4; /* 减去地址(4B) */
    //result = Flash_Verify(upgrade_addr, &data[4], g_upgrade_len);
		result=1;
    break;

  case CMD_IAP_EXIT: /* 跳转 App */
    result = 1;
    break;

  default:
    result = 0;
    break;
  }

  response[3] = result;
  response[4] = response[3] & 0x01; // 校验位只加数据

  /*  USB 发送状态检查与重试 */
  uint32_t start = HAL_GetTick();
  while ((CDC_Transmit_FS(response, 6) == USBD_BUSY) &&
         (HAL_GetTick() - start < 100)) // 100ms 超时
  {
    HAL_Delay(1);
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
