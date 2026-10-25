/**
 * 用于上位机和下位机沟通 */

#include "uart_task.h"
#include "UARTCommunication.h"
#include "usart.h"
#include "led.h"
#include "cmsis_os2.h"
#include "motor_task.h"
#include "CANCommunication.h"

// 与上位机连接
volatile bool upper_connected = false;
// 上次收到有效命令的时间
volatile uint32_t upper_time = 0;
// 上位机停止请求
volatile bool upper_stop = false;

// 最近一条有效命令是否为速度模式启动请求
volatile bool upper_start = false;

// 接收完成
#define UART_RX_DONE 1

// 串口发生错误
#define UART_RX_ERROR 2

// 任务线程id，供回调函数内设置flag
static osThreadId_t uart_handle;

// 接收缓冲区
static uint8_t rx_buffer[32];

// 本次收到的字节数（防止解析的时候解析到后面的垃圾数据）
static volatile uint16_t rx_length = 0;

// 启动一次DMA接收，开始接收（来自上位机的数据）
// 并且将数据存入rx_buffer
static bool StartReceive(void)
{
    // 开启DMA
    HAL_StatusTypeDef result = HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, sizeof(rx_buffer));

    if (result != HAL_OK)
    {
        // 接收失败
        return false;
    }

    // 每次启动后，关闭半传输中断
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

    return true;
}

// 串口任务入口
void UartTask(void *argument)
{
    (void)argument;

    uart_handle = osThreadGetId();

    // 初始化命令
    Command command = {0};

    // 上次发送时间
    uint32_t last_send = HAL_GetTick();

    // 清空接收的字节数
    UART_Reset();

    // 开启接收数据，把开启成功/失败的标志存入receving
    // 获得rx_buffer
    bool receiving = StartReceive();

    // 进入死循环
    while (1)
    {
        uint32_t flags = osThreadFlagsWait(UART_RX_DONE | UART_RX_ERROR, osFlagsWaitAny, 20);

        // 错误时，中断接收，清空状态，设置为未接收数据状态
        if (flags == UART_RX_ERROR || flags == 3)
        {
            HAL_UART_AbortReceive(&huart1);

            UART_Reset();

            receiving = false;
        }

        // 接收八字节成功（但数据不一定准确）
        else if (flags == UART_RX_DONE)
        {
            // 停止接收状态
            receiving = false;

            for (uint16_t i = 0; i < rx_length; i++)
            {
                // 对接收到的数据逐个解析，再判断返回结果（只有8字节了才返回true）
                bool frame_ok = UART_Input(rx_buffer[i], &command);

                if (frame_ok == true)
                {
                    // 只有完整有效帧才刷新连接时间
                    upper_time = HAL_GetTick();

                    // 任务二只用速度模式的A字段区分启动和停止
                    upper_start = command.mode == 0x00 && command.a > 0;

                    if (command.mode == 0x00)
                    {
                        upper_stop = command.a <= 0;
                    }

                    // 任意合法模式的数据包都能表明通信正常
                    upper_connected = true;

                    // 连接正常，开灯
                    Led_Set(true);
                }
            }
        }

        // 接收停止或者发生错误后，尝试重新开启
        if (receiving == false)
        {
            receiving = StartReceive();
        }

        uint32_t now = HAL_GetTick();

        // 没收到数据时，也必须检查连接是否超时
        if (upper_connected == true)
        {
            if ((uint32_t)(now - upper_time) >= UPPER_TIMEOUT_MS)
            {
                upper_connected = false;

                Led_Set(false);

                command = (Command){0};
                UART_Reset();
            }
        }

        // 没收到数据时，也继续发送反馈
        // 每20ms尝试通过DMA发送实际反馈
        if ((uint32_t)(now - last_send) >= 20)
        {
            last_send = now;

            // 实际输出轴速度，转换为协议要求的整数
            int16_t speed = (int16_t)motor_data.output_speed;

            // 多圈位置会持续累计，发送前限制在int16范围内
            float position_value = motor_data.position;

            if (position_value > 32767.0f)
            {
                position_value = 32767.0f;
            }
            else if (position_value < -32768.0f)
            {
                position_value = -32768.0f;
            }

            int16_t position = (int16_t)position_value;

            uint8_t state = motor_state;

            UART_Send(state, speed, position);
        }
    }
}

/*----------------------------------回调---------------------------------------*/
// 接收结束回调，只记录长度并置flag为1（接收完成）
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        rx_length = Size;

        osThreadFlagsSet(uart_handle, UART_RX_DONE);
    }
}

// 串口错误回调，通知任务恢复接收
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        osThreadFlagsSet(uart_handle, UART_RX_ERROR);
    }
}