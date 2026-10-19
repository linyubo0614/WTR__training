#include "uart_task.h"
#include "cmsis_os2.h"
#include "usart.h"
#include "data_types.h"
#include "queue_handles.h"

#include <string.h>
#include <stdio.h>

/* 枚举表示串口任务的三个状态 */
typedef enum
{
    UART_STATE_WAIT = 0,
    UART_STATE_SENDING = 1,
    UART_STATE_ERROR = 2
} UartTaskState;

volatile UartTaskState uart_state = UART_STATE_WAIT;

/* 发送中文状态提示 */
static void Uart_ReportState(const char *message)
{
    /* 串口发送当前状态 */
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)message,
        (uint16_t)strlen(message),
        100);
}

void UartTask(void *argument)
{
    (void)argument;

    SensorPacket packet;
    SensorData sample;

    /* 在不同状态之间保留待发送内容 */
    char text[96];
    int length = 0;

    for (;;)
    {
        switch (uart_state)
        {
        /* 状态一：等待数据，准备发送内容 */
        case UART_STATE_WAIT:
        {
            Uart_ReportState("串口状态:等待数据中\r\n");

            osStatus_t result = osMessageQueueGet(
                TxDateHandle,
                packet,
                NULL,
                osWaitForever);

            if (result != osOK)
            {
                uart_state = UART_STATE_ERROR;
                break;
            }

            /* 将9字节数据包还原为结构体 */
            sample.type = packet[0];

            memcpy(&sample.value,
                   &packet[1],
                   sizeof(sample.value));

            memcpy(&sample.time_ms,
                   &packet[5],
                   sizeof(sample.time_ms));

            /* 生成发送文字，vofa+中比较直观 */
            length = snprintf(
                text,
                sizeof(text),
                "sensor=%u value=%.2f time=%lu ms\r\n",
                (unsigned int)sample.type,
                (double)sample.value,
                (unsigned long)sample.time_ms);

            /* 检查格式化是否成功，缓冲区是否足够 */
            if (length <= 0 || length >= (int)sizeof(text))
            {
                uart_state = UART_STATE_ERROR;
                break;
            }

            /* 内容准备完成，下一轮执行发送 */
            uart_state = UART_STATE_SENDING;
            break;
        }

        /* 状态二：发送 */
        case UART_STATE_SENDING:
        {
            Uart_ReportState("串口状态:发送数据中\r\n");

            HAL_StatusTypeDef result = HAL_UART_Transmit(
                &huart1,
                (uint8_t *)text,
                (uint16_t)length,
                1000);

            if (result == HAL_OK)
            {
                uart_state = UART_STATE_WAIT;
            }
            else
            {
                uart_state = UART_STATE_ERROR;
            }

            break;
        }

        /* 状态三：记录错误，再尝试处理下一条数据 */
        case UART_STATE_ERROR:
        {
            Uart_ReportState("串口状态:错误!\r\n");

            uart_state = UART_STATE_WAIT;
            break;
        }

        default:
            uart_state = UART_STATE_ERROR;
            break;
        }
    }
}