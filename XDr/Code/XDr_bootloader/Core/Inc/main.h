/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/* ========== 固件升级配置 ========== */
/* 修改为 Sector 11 起始地址 (0x080E0000)，确保不会擦除 Bootloader */
/* F405 1MB Flash: Sector 11 地址范围 0x080E0000 - 0x080EFFFF */
#define FLAG_ADDRESS 0x080E0000
#define UPGRADE_MAGIC 0x12345678
#define NORMAL_MAGIC 0xFFFFFFFF

  uint8_t Check_Upgrade_Flag(void);
  void Set_Upgrade_Flag(void);
  void Clear_Upgrade_Flag(void);
  uint8_t Flash_Erase_App(void);
  uint8_t Flash_Write(uint32_t addr, uint8_t *data, uint16_t len);
  uint8_t Flash_Verify(uint32_t addr, uint8_t *data, uint16_t len);

  void JumpToApp(void);
  uint8_t Process_Upgrade_Cmd(uint8_t cmd, uint32_t addr, uint8_t *data, uint16_t len);
  /* USER CODE END ET */

  /* Exported constants --------------------------------------------------------*/
  /* USER CODE BEGIN EC */

  /* USER CODE END EC */

  /* Exported macro ------------------------------------------------------------*/
  /* USER CODE BEGIN EM */

  /* USER CODE END EM */

  /* Exported functions prototypes ---------------------------------------------*/
  void Error_Handler(void);

  /* USER CODE BEGIN EFP */

  /* USER CODE END EFP */

  /* Private defines -----------------------------------------------------------*/

  /* USER CODE BEGIN Private defines */

  /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
