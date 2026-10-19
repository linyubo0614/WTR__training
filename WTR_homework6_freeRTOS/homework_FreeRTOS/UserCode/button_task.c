#include "button_task.h"
#include "cmsis_os2.h"

void ButtonTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* 后续在这里检测PA5按键并消抖 */

        osDelay(10);
    }
}