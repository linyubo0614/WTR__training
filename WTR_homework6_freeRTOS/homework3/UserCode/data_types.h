#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <stdint.h>

/* 传感器采样数据结构体 */
//传感器数据
typedef struct
{
    uint8_t type;        /* 传感器编号 */
    float value;         /* 测量值 */
    uint32_t time_ms;    /* 时间戳，单位ms */
} SensorData;

//队列中传输的数据包类型，包含传感器编号、数据和时间戳
typedef uint8_t SensorPacket[9];

//之所以引入两个数据类型，是因为按作业要求在队列中传输的数据包和串口需要发送的数据包是一个字节数组，
//而在程序中处理的数据是一个结构体
//结构体更易于访问和操作，而字节数组更适合在队列中传输

#endif