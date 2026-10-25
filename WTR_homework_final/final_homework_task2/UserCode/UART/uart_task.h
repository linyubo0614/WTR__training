#ifndef UART_TASK_H
#define UART_TASK_H

#include <stdbool.h>
#include <stdint.h>

#define UPPER_TIMEOUT_MS 1500

extern volatile bool upper_connected;
extern volatile uint32_t upper_time;

// 上位机停止请求
extern volatile bool upper_stop;
// 最近一条有效命令是否为速度模式启动请求
extern volatile bool upper_start;

void UartTask(void *argument);

#endif
