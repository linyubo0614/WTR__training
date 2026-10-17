#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usart.h"

extern "C" void TaskA(void *argument);
extern "C" void TaskB(void *argument);
extern "C" void TaskC(void *argument);

volatile uint32_t counter = 0; // 共享计数器资源
osEventFlagsId_t event_flags;  // 事件标志
osThreadAttr_t thread_attr_A = {
    .name = "TaskA",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0,
    .stack_mem = NULL,
    .stack_size = 128 * 4, // 128 words
    .priority = osPriorityNormal,
};
osThreadAttr_t thread_attr_B = {
    .name = "TaskB",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0,
    .stack_mem = NULL,
    .stack_size = 128 * 4, // 128 words
    .priority = osPriorityNormal,
};
osThreadAttr_t thread_attr_C = {
    .name = "TaskC",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0,
    .stack_mem = NULL,
    .stack_size = 128 * 4, // 128 words
    .priority = osPriorityNormal,
};

extern "C" void Init(void *argument) {
  // 在这里面创建你自己的任务
  event_flags = osEventFlagsNew(NULL);
  osThreadNew(TaskA, NULL, &thread_attr_A);
  osThreadNew(TaskB, NULL, &thread_attr_B);
  osThreadNew(TaskC, NULL, &thread_attr_C);
  osThreadExit();
}

extern "C" void TaskA(void *argument) {

  for (uint32_t i = 0; i < 0x1000000; i++) {
    counter++; // 读-改-写，非原子
  }
  osEventFlagsSet(event_flags, 0x0001);
  osThreadExit();
}

extern "C" void TaskB(void *argument) {

  for (uint32_t i = 0; i < 0x1000000; i++) {
    counter++; // 读-改-写，非原子
  }
  osEventFlagsSet(event_flags, 0x0002);

  osThreadExit();
}

extern "C" void TaskC(void *argument) {
  osEventFlagsWait(event_flags, 0x0003, osFlagsWaitAll, osWaitForever);
  while (1) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&counter, sizeof(counter), 100);
    osDelay(1000);
  }
}
