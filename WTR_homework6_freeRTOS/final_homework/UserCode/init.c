#include "init.h"
#include "led.h"
#include "uart_task.h"
#include "main.h"
#include "cmsis_os2.h"
#include "CANCommunication.h"

void Init(void *argument)
{
    (void)argument;

    // 上位机尚未连接，先熄灭LED
    Led_Set(false);

    // 配置CAN并开启接收通知
    if (!CAN_Start())
    {
        Error_Handler();
    }
    
    osThreadExit();
}