#ifndef UART_TASK_H
#define UART_TASK_H

#include <stdbool.h>
#include <stdint.h>

#define UPPER_TIMEOUT_MS 1000

extern volatile bool upper_connected;
extern volatile uint32_t upper_time;

void UartTask(void *argument);

#endif
