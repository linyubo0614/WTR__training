#include "uart_task.h"
#include "UARTCommunication.h"
#include "motor_task.h"
#include "CANCommunication.h"
#include "usart.h"
#include "led.h"
#include "cmsis_os2.h"

// 设置标志，标记接收到数据
#define UART_RX_DONE 1

// 与上位机连接
volatile bool upper_time = false;

// 上次收到有效命令的时间
volatile uint32_t upper_time = 0;

// 任务线程id，供回调函数内设置flag
static osThreadId_t uart_handle;

// 接收缓冲区
static uint8_t rx_buffer[32];
// 本次收到的字节数（防止解析的时候解析到后面的垃圾数据）
static volatile uint16_t rx_length = 0;


// --------------------------------------启动DMA接收------------------------------
static bool StartReceive(void)
{
    HAL_StatusTypeDef result = HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, sizeof(rx_buffer));

    if (result != HAL_OK)
        return false;

    // 关闭传输过半中断
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    return true;
}

// ---------------------------------------串口空闲中断回调函数-------------------------------
// 只记录长度并通知任务
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        rx_length = Size;
        osThreadFlagsSet(uart_handle, UART_RX_DONE);
    }
}

// -----------------------------对上报上位机数据的初步处理---------------------------------
/**
 * 两字节int16，必须限幅不然装不下
 * 之后转化为int16，再发送给上位机
 * 
 * 需要说明的是，传输两个字节是受限于协议
 * 如果实际speed或者posision超过范围，上位机只能收到最大/最小值
 * 但是在PID计算的时候没有这个限制，仍然可以利用实际值
 * 所以影响不大
 */
static int16_t ToInt16(float value)
{
    if (value > 32767.0f)
        return 32767;
    if (value < -32768.0f)
        return -32768;
    return (int16_t)value;
}

//-----------------------------------------------串口任务---------------------------------
/**
 * 用来开启DMA
 * 将从上位机处接收到的命令传到电机任务
 * 将下位机从电调处得到的数据反馈给上位机
*/
void UartTask(void *argument)
{
    (void)argument;

    // 获取当前线程id
    uart_handle = osThreadGetId();

    // 初始化命令结构体
    Command command = {0};

    // 上次发送反馈的时间（第一次用当前时间初始化）
    uint32_t last_send = HAL_GetTick();

    // 清空已放入缓冲区的字节
    UART_Reset();

    // 如果开启了设为ture，false可以再次开始
    bool receiving = StartReceive();

    while (1)
    {
        uint32_t flags = osThreadFlagsWait(UART_RX_DONE , osFlagsWaitAny, 20);

        // 正常完成
        if (flags == UART_RX_DONE)
        {
            receiving = false;

            for (uint16_t i = 0; i < rx_length; i++)
            {
                // 一个个字节放入数据流解析
                bool frame_ok = UART_Input(rx_buffer[i], &command);
                if (frame_ok)
                {
                    Motor_SetCommand(&command);
                    Led_Set(true);
                }
            }
        }

        // 处理后如果可以开启，就尝试开启DMA
        // 同时如果开启失败就返回false，从而再次开启，实现自动重连
        if (receiving==false)
            receiving = StartReceive();
    
        uint32_t now = HAL_GetTick();

        // 如果上位机连接，但是超时了，认定为断开
        if (upper_connected && (uint32_t)(now - upper_time) >= UPPER_TIMEOUT_MS)
        {
            upper_connected = false;
            Led_Set(false);
            UART_Reset();
        }

        // 控制反馈发送间隔
        if ((uint32_t)(now - last_send) >= 20)
        {
            last_send = now;

            // 将当前速度与位置（来自电调）转化为16字节整数后传回串口
            int16_t speed = ToInt16(motor_data.output_speed);
            int16_t position = ToInt16(motor_data.position);
            UART_Send(motor_state, speed, position);
        }
    }
}

