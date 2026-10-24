#include "UARTCommunication.h"
#include "crc.h"
#include <stddef.h>
#include "usart.h"
#include <string.h>

/* --------------------------------------接收上位机数据--------------------------------------------- */

// 此函数用于拼接两个字节
static int16_t ReadInt16(const uint8_t *data)
{

    uint16_t value = (uint16_t)data[0] | ((uint16_t)data[1] << 8);

    // 负数情况
    if (value >= 0x8000)
    {
        return (int16_t)((int32_t)value - 65536);
    }

    return (int16_t)value;
}

// 检查并解析一帧上位机命令
// data通过串口获得，来自上位机
bool UART_Decode(const uint8_t *data, uint16_t length, Command *command)
{
    // 1. 检查输入参数
    if (data == NULL || command == NULL || length != 8)
    {
        return false;
    }

    // 2. 检查帧头
    if (data[0] != 0xAA || data[1] != 0xBB)
    {
        return false;
    }

    // 3. 检查CRC
    if (Crc8(data, 7) != data[7])
    {
        return false;
    }

    // 4. 检查模式是否在作业限定的三种之类
    if (data[2] != 0x00 && data[2] != 0x11 && data[2] != 0x22)
    {
        return false;
    }

    // 5. 检查通过后，再解析

    // 结构体
    Command result = {0};

    result.mode = data[2];

    // 预设模式
    if (result.mode == 0x22)
    {
        // 组合出预设动作id
        result.action_id = (uint32_t)data[3] | ((uint32_t)data[4] << 8) | ((uint32_t)data[5] << 16) | ((uint32_t)data[6] << 24);
    }
    else
    {
        // 获取A
        result.a = ReadInt16(&data[3]);

        // 获取B
        result.b = ReadInt16(&data[5]);
    }

    // 利用指针返回结果
    *command = result;

    return true;
}

/* --------------------------------------------打包反馈数据---------------------------------------------- */

// 将来自电调的反馈数据打包成上位机要求的8字节格式，存入数组，之后反馈给上位机
void UART_Encode(uint8_t state,
                 int16_t speed,
                 int16_t position,
                 uint8_t *frame)
{
    // 1. 帧头
    frame[0] = 0xBB;
    frame[1] = 0xAA;

    // 2. 反馈状态
    frame[2] = state;

    // 3. 速度，小端序
    frame[3] = (uint8_t)speed;
    frame[4] = (uint8_t)((uint16_t)speed >> 8);

    // 4. 位置
    frame[5] = (uint8_t)position;
    frame[6] = (uint8_t)((uint16_t)position >> 8);

    // 5. 计算CRC8(用上前面所有字节)
    frame[7] = Crc8(frame, 7);
}

/* ----------------------------------------发送反馈给上位机---------------------------------------------- */

// 通过DMA发送一帧反馈
int UART_Send(uint8_t state,
              int16_t speed,
              int16_t position)
{
    static uint8_t tx_frame[8];

    // 上一次未发送完，不覆盖发送数组
    if (huart1.gState != HAL_UART_STATE_READY)
    {
        return HAL_BUSY;
    }

    // 上一次发送完了，可以正常打包
    UART_Encode(state, speed, position, tx_frame);

    // 发送，同时返回状态
    return HAL_UART_Transmit_DMA(&huart1, tx_frame, sizeof(tx_frame));
}

/* ---------------------------------------处理上位机发送的数据流----------------------------------- */

// 目的是识别并且丢弃错误帧

// 保存正在检查的8字节候选帧
static uint8_t parse_frame[8];

// 当前已积累的字节数
static uint8_t parse_count = 0;

// 清空
void UART_Reset(void)
{
    parse_count = 0;
}

// 每次输入一个字节，尝试找出完整有效帧
bool UART_Input(uint8_t byte, Command *command)
{
    // 保存新字节
    parse_frame[parse_count] = byte;

    // 指针后移
    parse_count++;

    // 不足8字节，继续等待
    if (parse_count < sizeof(parse_frame))
    {
        return false;
    }

    // 积累够8字节，检查并且解码
    if (UART_Decode(parse_frame, sizeof(parse_frame), command))
    {
        // 已处理一帧，准备接收下一帧
        parse_count = 0;

        return true;
    }

    // 当前8字节不是有效帧
    // 只丢掉最前面的一个字节，保留后7字节
    // 把后七个字节复制到数组前面，第一个（最早的一个）丢弃
    memmove(parse_frame, &parse_frame[1], sizeof(parse_frame) - 1);

    parse_count = sizeof(parse_frame) - 1;

    return false;
}
