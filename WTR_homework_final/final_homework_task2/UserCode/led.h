#ifndef LED_H
#define LED_H

#include <stdbool.h>

//根据上位机连接状态设置LED
//传入布尔类型，表示连接状态，ture=连接；false=断开
void Led_Set(bool on);

#endif