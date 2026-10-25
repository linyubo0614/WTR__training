#ifndef CANCOMMUNICATION_H
#define CANCOMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>

// M2006减速比
#define MOTOR_RATIO 36.0f

// 转子和输出轴的正负方向
#define MOTOR_DIRECTION 1

// 作业规定的C610电流指令上限（对应4A）
#define MOTOR_CURRENT_LIMIT 4000

// 电调反馈的电机数据（含原始反馈和输出轴换算结果）
// 注：发送频率1KHz
typedef struct
{
    // 转子角度，无单位，范围：0-8191，对应0-360°
    uint16_t angle;

    // 转子转速，单位rpm，可以为负数
    int16_t speed;

    // 减速后的输出轴转速，单位rpm
    float output_speed;

    // 输出轴相对初始位置的角度，单位deg
    float position;

    // 最近一次收到有效反馈的时间（单位是ms）
    uint32_t time_ms;

    // 是否收到过有效反馈
    bool received;

} MotorData;

// 全局反馈数据，在CANCommunication.c中定义
extern volatile MotorData motor_data;

bool CAN_Start(void);

// 向电调（一号）发送电流指令
bool CAN_SendCurrent(int16_t current);

#endif