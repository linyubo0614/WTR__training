#include "button_task.h"
#include "cmsis_os2.h"
#include "main.h"
#include "process_task.h"

/* 调试观察：有效按下次数 */
volatile uint32_t button_press_count = 0;

void ButtonTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
        {
            osDelay(20);

            if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
            {
                button_press_count++;

                //执行状态切换
                Process_RequestModeSwitch();


                while (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
                {
                    osDelay(10);
                }
                osDelay(20);
            }
        }

        osDelay(10);
    }
}