/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LED.h"
#include "leg.h"
#include "usart.h"
#include "body.h"
#include "trot.h"
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
/* USER CODE BEGIN Variables */
float tempAngle = 0;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LEG_1 */
osThreadId_t LEG_1Handle;
const osThreadAttr_t LEG_1_attributes = {
  .name = "LEG_1",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LEG_2 */
osThreadId_t LEG_2Handle;
const osThreadAttr_t LEG_2_attributes = {
  .name = "LEG_2",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LEG_3 */
osThreadId_t LEG_3Handle;
const osThreadAttr_t LEG_3_attributes = {
  .name = "LEG_3",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LEG_4 */
osThreadId_t LEG_4Handle;
const osThreadAttr_t LEG_4_attributes = {
  .name = "LEG_4",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LEG_ALL */
osThreadId_t LEG_ALLHandle;
const osThreadAttr_t LEG_ALL_attributes = {
  .name = "LEG_ALL",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        if (BT_buffer == 0x01) {
            LED_free();
        }
    }
    HAL_UART_Receive_IT(&huart3, &BT_buffer, 1);
}
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void LEG_1_MOVE(void *argument);
void LEG_2_MOVE(void *argument);
void LEG_3_MOVE(void *argument);
void LEG_4_MOVE(void *argument);
void LEG_ALL_MOVE(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of LEG_1 */
  LEG_1Handle = osThreadNew(LEG_1_MOVE, NULL, &LEG_1_attributes);

  /* creation of LEG_2 */
  LEG_2Handle = osThreadNew(LEG_2_MOVE, NULL, &LEG_2_attributes);

  /* creation of LEG_3 */
  LEG_3Handle = osThreadNew(LEG_3_MOVE, NULL, &LEG_3_attributes);

  /* creation of LEG_4 */
  LEG_4Handle = osThreadNew(LEG_4_MOVE, NULL, &LEG_4_attributes);

  /* creation of LEG_ALL */
  LEG_ALLHandle = osThreadNew(LEG_ALL_MOVE, NULL, &LEG_ALL_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
    /* Infinite loop */
    for (;;) {

        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        osDelay(2000);

        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
        osDelay(1000);
    }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_LEG_1_MOVE */
/**
 * @brief Function implementing the LEG_1 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_LEG_1_MOVE */
void LEG_1_MOVE(void *argument)
{
  /* USER CODE BEGIN LEG_1_MOVE */
    /* Infinite loop */
    for (;;) {
//        LEG_stand(1);
//        osDelay(2000);
//        LEG_grovel(1);
        osDelay(2000);
    }
  /* USER CODE END LEG_1_MOVE */
}

/* USER CODE BEGIN Header_LEG_2_MOVE */
/**
 * @brief Function implementing the LEG_2 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_LEG_2_MOVE */
void LEG_2_MOVE(void *argument)
{
  /* USER CODE BEGIN LEG_2_MOVE */
    /* Infinite loop */
    for (;;) {
//        		LEG_stand(2);
//        		osDelay(2000);
//        		LEG_grovel(2);
            osDelay(2000);
    }
  /* USER CODE END LEG_2_MOVE */
}

/* USER CODE BEGIN Header_LEG_3_MOVE */
/**
 * @brief Function implementing the LEG_3 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_LEG_3_MOVE */
void LEG_3_MOVE(void *argument)
{
  /* USER CODE BEGIN LEG_3_MOVE */
    /* Infinite loop */
    for (;;) {
//        		LEG_stand(3);
//        		osDelay(2000);
//        		LEG_grovel(3);
            osDelay(2000);
    }
  /* USER CODE END LEG_3_MOVE */
}

/* USER CODE BEGIN Header_LEG_4_MOVE */
/**
 * @brief Function implementing the LEG_4 thread.
 * @param argument: Not used
 * @retval None
 */
float angle1 = 0;
float angle2 = 0;
/* USER CODE END Header_LEG_4_MOVE */
void LEG_4_MOVE(void *argument)
{
  /* USER CODE BEGIN LEG_4_MOVE */
    /* Infinite loop */
    for (;;) {
//	PWM_turn(htim3,7,angle1);
//	PWM_turn(htim3,8,angle1);
//	osDelay(2000);
		//LEG_grovel(4);
        //osDelay(2000);
//		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 50 + (90 + tempAngle) / 180 * 200);
//		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 50 + (90 + tempAngle) / 180 * 200);
//		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 50 + (90 + tempAngle) / 180 * 200);
//		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 50 + (90 + tempAngle) / 180 * 200);
		osDelay(10);
    }
  /* USER CODE END LEG_4_MOVE */
}

/* USER CODE BEGIN Header_LEG_ALL_MOVE */
/**
* @brief Function implementing the LEG_ALL thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LEG_ALL_MOVE */
void LEG_ALL_MOVE(void *argument)
{
  /* USER CODE BEGIN LEG_ALL_MOVE */
  /* Infinite loop */
  for(;;)
  {
		Calulate(&PosNew1, Runtime, &trot1);
		Calulate(&PosNew2, Runtime, &trot2);
		Calulate(&PosNew3, Runtime, &trot3);
		Calulate(&PosNew4, Runtime, &trot4);
		InverseKinematics(&PosNew1, &body.leg[0]);
		InverseKinematics(&PosNew2, &body.leg[1]);
		InverseKinematics(&PosNew3, &body.leg[2]);
		InverseKinematics(&PosNew4, &body.leg[3]);
		LegControl(body);
		
    osDelay(10);
  }
  /* USER CODE END LEG_ALL_MOVE */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

