#include "sensor.h"
#include "data_types.h"
#include "cmsis_os2.h"
#include "main.h"
#include "queue_handles.h"

#include <string.h>
#include <stdlib.h>

/* 两个软件定时器的句柄 */
static osTimerId_t sensor1_timer;
static osTimerId_t sensor2_timer;

/* 定时器到期后，由FreeRTOS调用这个回调函数 */
/* 传感器1回调函数 */
static void Sensor1_Callback(void *argument)
{
    (void)argument;

    /* 生成本次采样数据 */
    SensorData sample;

    /* 编号为1 */
    sample.type = 1;
    sample.value = (float)(rand() % 10001) / 100.0f;
    sample.time_ms = HAL_GetTick();

    /* 将三个字段放进9字节数据包 */
    SensorPacket packet;

    /* 传感器编号占1字节，测量值占4字节，时间戳占4字节 */
    packet[0] = sample.type;

    memcpy(
        &packet[1],
        &sample.value,
        sizeof(sample.value));

    memcpy(
        &packet[5],
        &sample.time_ms,
        sizeof(sample.time_ms));

    /* 放入传感器1队列 */
    osMessageQueuePut(
        Sensor01Handle, // 队列句柄
        packet,
        0,
        0);
}

/* 传感器2回调函数 */
static void Sensor2_Callback(void *argument)
{
    (void)argument;

    /* 生成数据 */
    SensorData sample;

    sample.type = 2;
    sample.value = (float)(rand() % 10001) / 100.0f;
    sample.time_ms = HAL_GetTick();

    /* 整理成9字节的数据包 */
    SensorPacket packet;

    packet[0] = sample.type;

    memcpy(&packet[1],
           &sample.value,
           sizeof(sample.value));

    memcpy(&packet[5],
           &sample.time_ms,
           sizeof(sample.time_ms));

    /* 放入第二路传感器自己的队列，不等待 */
    osMessageQueuePut(
        Sensor02Handle,
        packet,
        0,
        0);
}

/* 启动传感器 */
/* 创建并启动两路传感器的软件定时器 */
void Sensor_Start(void)
{
    /* 创建第一路定时器 */
    sensor1_timer = osTimerNew(
        Sensor1_Callback,
        osTimerPeriodic,
        NULL,
        NULL);

    /* 创建第二路定时器 */
    sensor2_timer = osTimerNew(
        Sensor2_Callback,
        osTimerPeriodic,
        NULL,
        NULL);

    /* 任意一个创建失败，都停止并进入错误处理 */
    if (sensor1_timer == NULL || sensor2_timer == NULL)
    {
        Error_Handler();
    }

    /* 配置每秒1000个tick，因此100tick = 100ms */
    if (osTimerStart(sensor1_timer, 100U) != osOK)
    {
        Error_Handler();
    }

    /* 配置每秒1000个tick，因此200tick = 200ms */
    if (osTimerStart(sensor2_timer, 200U) != osOK)
    {
        Error_Handler();
    }
}