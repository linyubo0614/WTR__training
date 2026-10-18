#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usart.h"
#include <cstring>

extern "C" void TaskA(void *argument);
extern "C" void TaskB(void *argument);
extern "C" void TaskC(void *argument);

// 延时us数
//100，240，300，350，500
constexpr uint32_t WRITE_WAIT_US = 10000U;

static void BusyWaitBeforeWrite(uint32_t wait_us) {
  const uint32_t cycles_per_us = osKernelGetSysTimerFreq() / 1000000U;
  const uint32_t wait_cycles = cycles_per_us * wait_us;
  const uint32_t start = osKernelGetSysTimerCount();

  while ((uint32_t)(osKernelGetSysTimerCount() - start) < wait_cycles) {
    __NOP();
  }
}

constexpr uint32_t BUFFER_SIZE = 100U;
volatile uint8_t shared_buffer[BUFFER_SIZE];
osEventFlagsId_t event_flags;
osThreadAttr_t thread_attr_A = {
    .name = "TaskA",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0,
    .stack_mem = NULL,
    .stack_size = 128 * 4, // 128 words
    .priority = osPriorityHigh,
};
osThreadAttr_t thread_attr_B = {
    .name = "TaskB",
    .attr_bits = osThreadDetached,
    .cb_mem = NULL,
    .cb_size = 0,
    .stack_mem = NULL,
    .stack_size = 128 * 4, // 128 words
    .priority = osPriorityHigh,
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
  std::memset((void *)shared_buffer, '.', sizeof(shared_buffer));

  event_flags = osEventFlagsNew(NULL);
  osThreadNew(TaskA, NULL, &thread_attr_A);
  osThreadNew(TaskB, NULL, &thread_attr_B);
  osThreadNew(TaskC, NULL, &thread_attr_C);
  osThreadExit();
}

extern "C" void TaskA(void *argument) {
  BusyWaitBeforeWrite(WRITE_WAIT_US);
  std::memset((void *)shared_buffer, 'A', sizeof(shared_buffer));
  osEventFlagsSet(event_flags, 0x0001);
  osThreadExit();
}

extern "C" void TaskB(void *argument) {
  std::memset((void *)shared_buffer, 'B', sizeof(shared_buffer));
  osEventFlagsSet(event_flags, 0x0002);
  osThreadExit();
}

extern "C" void TaskC(void *argument) {
  osEventFlagsWait(event_flags, 0x0003, osFlagsWaitAll, osWaitForever);
  HAL_UART_Transmit(&huart1, (uint8_t *)shared_buffer, BUFFER_SIZE, 1000);
  osThreadExit();
}