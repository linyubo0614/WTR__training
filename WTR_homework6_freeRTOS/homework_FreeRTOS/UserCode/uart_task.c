#include "uart_task.h"
#include "cmsis_os2.h"

/* 串口任务 */
void UartTask(void *argument)
{
    /* 当前暂时不用任务参数 */
    (void)argument;

    for (;;)
    {
        /* 后续在这里加入串口发送功能 */

        osDelay(10);
    }
}