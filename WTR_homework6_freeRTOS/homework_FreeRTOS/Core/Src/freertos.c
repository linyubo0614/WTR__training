/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "data_types.h"
#include "sensor.h"
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

/* USER CODE END Variables */
/* Definitions for initTask */
osThreadId_t initTaskHandle;
const osThreadAttr_t initTask_attributes = {
    .name = "initTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for processTask */
osThreadId_t processTaskHandle;
const osThreadAttr_t processTask_attributes = {
    .name = "processTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for uartTask */
osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
    .name = "uartTask",
    .stack_size = 384 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for buttonTask */
osThreadId_t buttonTaskHandle;
const osThreadAttr_t buttonTask_attributes = {
    .name = "buttonTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for Sensor01 */
osMessageQueueId_t Sensor01Handle;
const osMessageQueueAttr_t Sensor01_attributes = {
    .name = "Sensor01"};
/* Definitions for Sensor02 */
osMessageQueueId_t Sensor02Handle;
const osMessageQueueAttr_t Sensor02_attributes = {
    .name = "Sensor02"};
/* Definitions for TxDate */
osMessageQueueId_t TxDateHandle;
const osMessageQueueAttr_t TxDate_attributes = {
    .name = "TxDate"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Init(void *argument);
extern void ProcessTask(void *argument);
extern void UartTask(void *argument);
extern void ButtonTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
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

  /* Create the queue(s) */
  /* creation of Sensor01 */
  Sensor01Handle = osMessageQueueNew(16, sizeof(SensorPacket), &Sensor01_attributes);

  /* creation of Sensor02 */
  Sensor02Handle = osMessageQueueNew(16, sizeof(SensorPacket), &Sensor02_attributes);

  /* creation of TxDate */
  TxDateHandle = osMessageQueueNew(16, sizeof(SensorPacket), &TxDate_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of initTask */
  initTaskHandle = osThreadNew(Init, NULL, &initTask_attributes);

  /* creation of processTask */
  processTaskHandle = osThreadNew(ProcessTask, NULL, &processTask_attributes);

  /* creation of uartTask */
  uartTaskHandle = osThreadNew(UartTask, NULL, &uartTask_attributes);

  /* creation of buttonTask */
  buttonTaskHandle = osThreadNew(ButtonTask, NULL, &buttonTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_Init */
/**
 * @brief  Function implementing the initTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_Init */
void Init(void *argument)
{
  /* USER CODE BEGIN Init */
  (void)argument;

  Sensor_Start();

  osThreadExit();

  /* USER CODE END Init */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
