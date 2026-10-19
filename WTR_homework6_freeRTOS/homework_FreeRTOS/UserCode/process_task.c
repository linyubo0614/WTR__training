#include "process_task.h"
#include "cmsis_os2.h"

void ProcessTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* 后续在这里读取传感器数据并进行处理 */

        osDelay(10);
    }
}