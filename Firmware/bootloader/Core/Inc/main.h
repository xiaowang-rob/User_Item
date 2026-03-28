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
#include "stdbool.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/* ========== Flash 地址规划 ========== */
#define APP_START_ADDR 0x08004000U    // App 起始地址 (Sector 1)
#define FLASH_END_ADDR 0x080FFFFFU    // F405 1MB Flash 结束地址
#define CONFIG_SECTOR FLASH_SECTOR_11 // 配置扇区 (存升级标志)，不擦除
  /* ========== 固件升级配置 ========== */
  /* 修改为 Sector 11 起始地址 (0x080E0000)，确保不会擦除 Bootloader */
  /* F405 1MB Flash: Sector 11 地址范围 0x080E0000 - 0x080EFFFF */

#define FLAG_ADDRESS 0x080E0000
#define NORMAL_MAGIC 0xFFFFFFFF

#define CMD_BL_CONNECT 0x3f       // 连接Bootloader
#define CMD_IAP_ENTER 0x31        // 进入IAP模式 进入开始固件烧录确认
#define CMD_IAP_ERASE_FLASH 0x32  // 擦除flash
#define CMD_IAP_WRITE_FLASH 0x33  // 写入flash
#define CMD_IAP_VERIFY_FLASH 0x34 // 校验flash
#define CMD_IAP_EXIT 0x35         // 完成 退出IAP模式 进入APP

#define FEEDBACK_OK 0xf0
#define FEEDBACK_ERROR 0xfe

  /* 升级命令全局变量 (usbd_cdc_if.c 中定义) */
  extern volatile uint8_t g_upgrade_cmd;
  extern volatile uint16_t g_upgrade_len;
  extern volatile uint8_t g_upgrade_data[256];
  extern volatile uint8_t g_cmd_received;

  /* === 新增：固件大小和擦除扇区信息 === */
  extern volatile uint32_t g_firmware_total_size; // 固件总大小
  extern volatile uint8_t g_erase_sectors_start;  // 起始扇区号
  extern volatile uint8_t g_erase_sectors_count;  // 擦除扇区数量

  bool Check_Upgrade_Flag(void);
  void Set_Upgrade_Flag(void);
  bool Clear_Upgrade_Flag(void);
  bool Flash_Erase_App(void);
  bool Flash_Write(uint32_t addr, uint8_t *data, uint16_t len);
  bool Flash_Verify(uint32_t addr, uint8_t *data, uint16_t len);

  void JumpToApp(void);
  uint8_t Process_Upgrade_Cmd(uint8_t cmd, uint8_t *data, uint16_t len);
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
