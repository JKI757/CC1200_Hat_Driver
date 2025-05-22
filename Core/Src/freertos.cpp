/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "iwdg.h"
#include "usb_device.h"
#include "globals.h"
#include "Radio.h"
#include "VCPMenu.h"
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
// Pointer to the globals object passed from main
static Globals* g_globals = nullptr;

// VCP Menu instance
static VCPMenu* g_vcpMenu = nullptr;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTask02 */
osThreadId_t myTask02Handle;
const osThreadAttr_t myTask02_attributes = {
  .name = "myTask02",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTask03 */
osThreadId_t myTask03Handle;
const osThreadAttr_t myTask03_attributes = {
  .name = "myTask03",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTask04 */
osThreadId_t myTask04Handle;
const osThreadAttr_t myTask04_attributes = {
  .name = "myTask04",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myQueue01 */
osMessageQueueId_t myQueue01Handle;
const osMessageQueueAttr_t myQueue01_attributes = {
  .name = "myQueue01"
};
/* Definitions for myQueue02 */
osMessageQueueId_t myQueue02Handle;
const osMessageQueueAttr_t myQueue02_attributes = {
  .name = "myQueue02"
};
/* Definitions for myTimer01 */
osTimerId_t myTimer01Handle;
const osTimerAttr_t myTimer01_attributes = {
  .name = "myTimer01"
};
/* Definitions for myMutex01 */
osMutexId_t myMutex01Handle;
const osMutexAttr_t myMutex01_attributes = {
  .name = "myMutex01"
};
/* Definitions for myMutex02 */
osMutexId_t myMutex02Handle;
const osMutexAttr_t myMutex02_attributes = {
  .name = "myMutex02"
};
/* Definitions for myBinarySem01 */
osSemaphoreId_t myBinarySem01Handle;
const osSemaphoreAttr_t myBinarySem01_attributes = {
  .name = "myBinarySem01"
};
/* Definitions for myCountingSem01 */
osSemaphoreId_t myCountingSem01Handle;
const osSemaphoreAttr_t myCountingSem01_attributes = {
  .name = "myCountingSem01"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

// Task function prototypes with properly typed parameters
void StartDefaultTask(Globals* globals);
void StartTask02(Globals* globals);
void StartTask03(Globals* globals);
void StartTask04(Globals* globals);
void Callback01(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(Globals* globals); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  globals: Pointer to the globals object
  * @retval None
  */
void MX_FREERTOS_Init(Globals* globals) {
  // Store the globals pointer for use in this file
  g_globals = globals;
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of myMutex01 */
  myMutex01Handle = osMutexNew(&myMutex01_attributes);

  /* creation of myMutex02 */
  myMutex02Handle = osMutexNew(&myMutex02_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of myBinarySem01 */
  myBinarySem01Handle = osSemaphoreNew(1, 0, &myBinarySem01_attributes);

  /* creation of myCountingSem01 */
  myCountingSem01Handle = osSemaphoreNew(2, 2, &myCountingSem01_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of myTimer01 */
  myTimer01Handle = osTimerNew(Callback01, osTimerPeriodic, NULL, &myTimer01_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of myQueue01 */
  myQueue01Handle = osMessageQueueNew (16, sizeof(uint16_t), &myQueue01_attributes);

  /* creation of myQueue02 */
  myQueue02Handle = osMessageQueueNew (16, sizeof(uint16_t), &myQueue02_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew((osThreadFunc_t)StartDefaultTask, g_globals, &defaultTask_attributes);

  /* creation of myTask02 */
  myTask02Handle = osThreadNew((osThreadFunc_t)StartTask02, g_globals, &myTask02_attributes);

  /* creation of myTask03 */
  myTask03Handle = osThreadNew((osThreadFunc_t)StartTask03, g_globals, &myTask03_attributes);

  /* creation of myTask04 */
  myTask04Handle = osThreadNew((osThreadFunc_t)StartTask04, g_globals, &myTask04_attributes);

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
void StartDefaultTask(Globals* globals)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  // Initialize the Radio
  if (!globals->initRadio()) {
    // Radio initialization failed
    // Flash service LED rapidly to indicate error
    for (int i = 0; i < 10; i++) {
      globals->setServiceLED(1);
      osDelay(100);
      globals->setServiceLED(0);
      osDelay(100);
    }
  }
  
  // Create and initialize VCP Menu
  g_vcpMenu = new VCPMenu(globals);
  g_vcpMenu->init();
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
    
    // Process VCP Menu commands
    g_vcpMenu->processCommands();
    
    // Toggle service LED every second to indicate system is running
    static uint32_t lastToggle = 0;
    if (osKernelGetTickCount() - lastToggle > 1000) {
      static uint8_t ledState = 0;
      globals->setServiceLED(ledState);
      ledState = !ledState;
      lastToggle = osKernelGetTickCount();
    }
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(Globals* globals)
{
  /* USER CODE BEGIN StartTask02 */
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
    globals->refreshWatchdog(); // Use the globals object to refresh the watchdog
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(Globals* globals)
{
  /* USER CODE BEGIN StartTask03 */
  // This task handles radio transmission
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(10); // 10ms delay
    
    // Get the Radio instance
    Radio* radio = globals->getRadio();
    if (radio != nullptr) {
      // Check if there's data to transmit from a queue or buffer
      // For now, this is handled by the VCP Menu commands
      
      // Update TX LED based on radio state
      // This will be handled by the VCP Menu for now
    }
  }
  /* USER CODE END StartTask03 */
}

/* USER CODE BEGIN Header_StartTask04 */
/**
* @brief Function implementing the myTask04 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask04 */
void StartTask04(Globals* globals)
{
  /* USER CODE BEGIN StartTask04 */
  // This task handles radio reception
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(10); // 10ms delay
    
    // Get the Radio instance
    Radio* radio = globals->getRadio();
    if (radio != nullptr) {
      // Check for received data
      // For now, this is handled by the VCP Menu commands
      
      // Update RX LED based on radio state
      // This will be handled by the VCP Menu for now
    }
  }
  /* USER CODE END StartTask04 */
}

/* Callback01 function */
void Callback01(void *argument)
{
  /* USER CODE BEGIN Callback01 */

  /* USER CODE END Callback01 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

