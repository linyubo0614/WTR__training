#include "init.h"
#include "led.h"
#include "uart_task.h"
#include "main.h"
#include "cmsis_os2.h"
#include "CANCommunication.h"

void Init(void *argument)
{
    (void)argument;

    // 还没连接先熄灭LED
    Led_Set(false);

    // 开启CAN1
    if (!CAN_Start())
    {
        Error_Handler();
    }
    
    osThreadExit();
}