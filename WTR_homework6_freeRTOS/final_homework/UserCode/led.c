#include "led.h"
#include "main.h"

void Led_Set(bool on)
{

    //根据返回的布尔类型开关灯
    if (on)
    {
        HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_SET)
    }

    else if (on) 
    {
        HAL_GPIO_WritePin(LED0_GPIO_Port,LED0_Pin,GPIO_PIN_RESET)
    }
    
}