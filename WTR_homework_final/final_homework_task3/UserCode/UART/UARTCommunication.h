#ifndef UARTCOMMUNICATION_H
#define UARTCOMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>

/* -----------------接收上位机数据---------------- */

// 上位机的命令
typedef struct
{
    // 三个模式
    uint8_t mode;

    // 速度模式：最大输出电流（力矩）
    // 位置模式：最大输出速度
    int16_t a;

    // 速度模式：目标速度
    // 位置模式：控制位置
    int16_t b;

    uint32_t action_id; // 只有在预设动作模式才要用
} Command;


/* -----------------打包命令---------------- */
// 数据来自上位机
// 检查并解析一帧数据，成功返回ture，失败返回false
// 同时利用指针返回命令结构体
bool UART_Decode(const uint8_t *data, uint16_t length, Command *command);

// 将电机反馈的数据打包，之后发送给上位机
void UART_Encode(uint8_t state,      //反馈状态S
                 int16_t speed,      //反馈速度 
                 int16_t position,   //反馈位置
                 uint8_t *frame      //用于保存结果的数组
                );


/* -----------------发送反馈给上位机---------------- */

// 通过USART1的DMA发送一帧反馈数据（数据从打包好的数组取出即可）
// 后续只由串口任务调用
int UART_Send(uint8_t state,        //状态
               int16_t speed,        //速度
               int16_t position      //位置
            );


/* -----------------查找来自上位机的有效帧---------------- */

// 输入一个字节，找到有效帧时返回true
// 成功时，将解析结果写入command
bool UART_Input(uint8_t byte, Command *command);

// 清空尚未完成的组帧数据
void UART_Reset(void);

#endif