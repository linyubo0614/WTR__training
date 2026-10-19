#ifndef QUEUE_HANDLES_H
#define QUEUE_HANDLES_H

#include "cmsis_os2.h"

/* 这个队列句柄已经在freertos.c中定义 */
extern osMessageQueueId_t Sensor01Handle;
extern osMessageQueueId_t Sensor02Handle;
extern osMessageQueueId_t TxDateHandle;

#endif