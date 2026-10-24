#ifndef UART_TASK_H
#define UART_TASK_H

#include <stdbool.h>
#include <stdint.h>

// 判读上位机连接是否超时（多一些作冗余），单位ms
#define HOST_TIMEOUT_MS 1500

// 判断是否和上位机建立通信
extern volatile bool host_connected;

// 最近一次收到有效帧的时间，用于判断断连
extern volatile uint32_t host_time;

// 是否收到上位机的急停指令（但是不等于电机停了）
extern volatile bool host_stop;

void UartTask(void *argument);

#endif