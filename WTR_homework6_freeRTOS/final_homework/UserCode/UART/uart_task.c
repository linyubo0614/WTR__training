#include "uart_task.h"
#include "UARTCommunication.h"
#include "usart.h"
#include "led.h"
#include "cmsis_os2.h"

// 第0位：接收完成
#define UART_RX_DONE 1

// 第1位：串口发生错误
#define UART_RX_ERROR 2

// 串口任务句柄
static osThreadId_t uart_handle;

// DMA接收缓冲区
static uint8_t rx_buffer[32];

// 本次接收的字节数
static volatile uint16_t rx_length = 0;

// 启动一次DMA接收，开始接收（来自上位机的数据）
static bool StartReceive(void)
{
    HAL_StatusTypeDef result =
        HAL_UARTEx_ReceiveToIdle_DMA(
            &huart1,
            rx_buffer,
            sizeof(rx_buffer));

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

    // 最近一次解析成功的命令
    Command command = {0};

    // 上位机连接状态
    bool connected = false;

    // 最近一次收到有效命令的时刻
    uint32_t last_receive = 0;

    // 上次发送反馈的时刻
    uint32_t last_send = osKernelGetTickCount();

    // 尚未取得电机反馈时的初始状态
    uint8_t state = 0xFF;
    int16_t speed = 0;
    int16_t position = 0;

    UART_Reset();

    // 启动第一次接收
    // receiving用于保存接收状态
    bool receiving = StartReceive();

    while (1)
    {
        // 等待接收完成或错误通知（有其中一个就可以）
        // 3表示同时等待第0位和第1位
        uint32_t flags =osThreadFlagsWait(UART_RX_DONE | UART_RX_ERROR,osFlagsWaitAny,20);

        // 如果错误或者错误且完成接收
        if (flags == UART_RX_ERROR || flags == 3)
        {
            // 停止接收
            HAL_UART_AbortReceive(&huart1);

            // 硬件接收错误，重新开始接收
            UART_Reset();

            receiving = false;
        }


        // 如果完成而无错误
        else if (flags == UART_RX_DONE)
        {
            // 普通DMA模式下，本次接收已经停止
            receiving = false;

            // 逐字节处理数据
            for (uint16_t i = 0; i < rx_length; i++)
            {
                if (UART_Input(rx_buffer[i], &command))
                {
                    // 只有完整有效帧才能刷新连接时间
                    last_receive = osKernelGetTickCount();

                    connected = true;

                    // 能正常接收，表明连接正常，开灯
                    Led_Set(true);
                }
            }
        }

        // 数据处理完成后，开启下一次接收
        // 如果启动失败，下一轮继续尝试（自动重连）
        if (receiving==false)
        {
            // 一直尝试，知道可以
            receiving = StartReceive();
        }

        uint32_t now = osKernelGetTickCount();


        // 上位机定时发送数据，据此检测上位机是否连接
        // 1.5秒没有收到有效命令，认为上位机断开（正常上位机最长不会超过1000ms没有发送）
        if (connected && (uint32_t)(now - last_receive) >= 1500)
        {
            connected = false;

            // 关灯
            Led_Set(false);

            // 清除旧命令
            command = (Command){0};

            // 清除未完成的旧半帧
            UART_Reset();
        }


        // 每20ms尝试发送一次当前反馈
        if ((uint32_t)(now - last_send) >= 20)
        {
            // 重置时间
            last_send = now;

            UART_Send(state, speed, position);
        }
    }
}

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