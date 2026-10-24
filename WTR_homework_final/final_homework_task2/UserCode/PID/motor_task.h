#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include <stdint.h>

extern volatile uint8_t motor_state;

// 电机控制任务入口
void MotorTask(void *argument);

#endif