#ifndef MOTOR_TASK_H
#define MOTOR_TASK_H

#include "UARTCommunication.h"

// 协议反馈状态：AA/AF为速度模式，BA/BF为位置模式，FF为CAN异常
extern volatile uint8_t motor_state;

// 只由串口任务调用，接收校验通过的命令
void Motor_SetCommand(const Command *command);
void MotorTask(void *argument);

#endif
