#pragma once
// 三份作业分别复制到三个工程。这里的配置相同，共用根目录的 .ioc。
#include "main.h"
#include "usart.h"
#include "cmsis_os2.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

// USART1：PA9=TX，PA10=RX，115200，8N1。
// 只观察输出时，PA9 接 USB 转串口模块 RX，双方 GND 相连即可。
#define APP_UART huart1

// PB0 配为 GPIO_Input + Pull-up。按钮连接 PB0 与 GND，按下是低电平。
#define APP_BUTTON_PORT GPIOB
#define APP_BUTTON_PIN GPIO_PIN_0
#define APP_BUTTON_PRESSED GPIO_PIN_RESET

// 很多 F103 小板的 PC13 LED 是低电平点亮。这里用它显示致命错误。
#define APP_LED_PORT GPIOC
#define APP_LED_PIN GPIO_PIN_13

// CMSIS 的 osDelay/osTimerStart 参数是 tick，不直接保证是毫秒。
// 向上取整：比如 tick=100Hz，15ms 需要至少 2 tick。
inline uint32_t AppMsToTicks(uint32_t ms) {
    if (ms == 0U) return 0U;
    const uint64_t ticks = (static_cast<uint64_t>(ms) * osKernelGetTickFreq()
                            + 999U) / 1000U;
    return static_cast<uint32_t>(ticks == 0U ? 1U : ticks);
}

inline osThreadAttr_t AppThreadAttr(const char *name, osPriority_t priority,
                                  uint32_t stack_bytes) {
    osThreadAttr_t attr = {}; // 全部清零，不使用 C++20 的指定初始化语法。
    attr.name = name;
    attr.priority = priority;
    attr.stack_size = stack_bytes; // CMSIS-RTOS2 这里的单位是字节。
    return attr;
}

// 只从任务上下文调用。失败时打印原因，LED 常亮，停在可调试的位置。
// 不在临界区调用，也不在硬件中断回调中调用。
inline void AppFatal(const char *reason) {
    HAL_GPIO_WritePin(APP_LED_PORT, APP_LED_PIN, GPIO_PIN_RESET);
    HAL_UART_Transmit(&APP_UART,
        reinterpret_cast<uint8_t *>(const_cast<char *>(reason)),
        static_cast<uint16_t>(std::strlen(reason)), 200U);
    for (;;) osDelay(AppMsToTicks(1000U));
}

inline void AppCheck(bool condition, const char *reason) {
    if (!condition) AppFatal(reason);
}
