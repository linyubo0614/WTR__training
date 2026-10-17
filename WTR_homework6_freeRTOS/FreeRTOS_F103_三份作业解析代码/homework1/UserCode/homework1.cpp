/* 作业一：先复现，再修复 counter++ 的共享资源冲突。
 * 只将本文件和 app_config.h 放进本工程的 UserCode。
 * CubeMX 中 InitTask 的 Entry Function 填 init，Code Generation 选 As external。
 * 先用 MODE=0 做观察，再用 MODE=1 验证正确结果。
 */
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"

// 0=课件的竞争写法；1=短临界区修复（默认）；2=互斥量修复。
// 每次改这个值后，重新编译、下载、复位。
#ifndef HW1_FIX_MODE
#define HW1_FIX_MODE 1
#endif

// 0=原代码的4字节二进制；1=方便初学者阅读的十进制文本（默认）。
// 只改变最终显示方式，不改变两个计算任务。
#ifndef HW1_TEXT_OUTPUT
#define HW1_TEXT_OUTPUT 1
#endif

#if HW1_FIX_MODE < 0 || HW1_FIX_MODE > 2
#error "HW1_FIX_MODE must be 0, 1 or 2"
#endif

static constexpr uint32_t LOOP_COUNT = 0x01000000U; // 16777216 次。
static constexpr uint32_t EXPECTED = 2U * LOOP_COUNT; // 33554432=0x02000000。
static constexpr uint32_t A_DONE = 0x01U;
static constexpr uint32_t B_DONE = 0x02U;

// volatile 要求真实读写内存，但不会把 ++ 变成不可分割的操作。
volatile uint32_t counter = 0U;
static osEventFlagsId_t done_flags = nullptr;
#if HW1_FIX_MODE == 2
static osMutexId_t counter_mutex = nullptr;
#endif

static void IncrementCounter() {
#if HW1_FIX_MODE == 0
    counter++; // 故意保留问题：读旧值、加1、写回之间可能切换任务。
#elif HW1_FIX_MODE == 1
    taskENTER_CRITICAL();
    counter++; // 临界区只有这一句。保护两位写入者的整个读改写。
    taskEXIT_CRITICAL();
    // 不要在上面两行之间放 osDelay、队列、串口或打印函数。
#else
    AppCheck(osMutexAcquire(counter_mutex, osWaitForever) == osOK,
             "HW1 mutex acquire failed\r\n");
    counter++;
    AppCheck(osMutexRelease(counter_mutex) == osOK,
             "HW1 mutex release failed\r\n");
    // 互斥量会有额外开销，但更适合保护较长的任务间资源访问。
#endif
}

static void TaskA(void *argument) {
    (void)argument; // 不用这个参数，避免编译器警告。
    for (uint32_t i = 0U; i < LOOP_COUNT; ++i) IncrementCounter();
    AppCheck((osEventFlagsSet(done_flags, A_DONE) & osFlagsError) == 0U,
             "HW1 A flag failed\r\n");
    osThreadExit(); // 有限任务必须显式退出，不直接 return。
}

static void TaskB(void *argument) {
    (void)argument;
    for (uint32_t i = 0U; i < LOOP_COUNT; ++i) IncrementCounter();
    AppCheck((osEventFlagsSet(done_flags, B_DONE) & osFlagsError) == 0U,
             "HW1 B flag failed\r\n");
    osThreadExit();
}

static void TaskC(void *argument) {
    (void)argument;
    // WaitAll 必须同时等到 A_DONE 和 B_DONE，才说明计算都完成了。
    const uint32_t flags = osEventFlagsWait(done_flags, A_DONE | B_DONE,
                                          osFlagsWaitAll, osWaitForever);
    AppCheck((flags & osFlagsError) == 0U, "HW1 wait failed\r\n");
    for (;;) {
        const uint32_t snapshot = counter; // 此时 A、B 已不再写，可以稳定读取。
#if HW1_TEXT_OUTPUT
        char line[112] = {};
        const int n = std::snprintf(line, sizeof(line),
            "HW1 mode=%u counter=%lu expected=%lu result=%s\r\n",
            static_cast<unsigned>(HW1_FIX_MODE),
            static_cast<unsigned long>(snapshot),
            static_cast<unsigned long>(EXPECTED),
            snapshot == EXPECTED ? "PASS" : "LOST_UPDATE");
        AppCheck(n > 0 && n < static_cast<int>(sizeof(line)),
                 "HW1 format failed\r\n");
        HAL_UART_Transmit(&APP_UART, reinterpret_cast<uint8_t *>(line),
                          static_cast<uint16_t>(n), 200U);
#else
        // F103 是小端。正确结果在十六进制接收中显示 00 00 00 02。
        HAL_UART_Transmit(&APP_UART,
            reinterpret_cast<uint8_t *>(const_cast<uint32_t *>(&snapshot)),
            sizeof(snapshot), 200U);
#endif
        osDelay(AppMsToTicks(1000U));
    }
}

extern "C" void init(void *argument) {
    (void)argument;
    // InitTask 必须高于下面的任务，否则建任务时就可能立刻切换出去。
    done_flags = osEventFlagsNew(nullptr);
    AppCheck(done_flags != nullptr, "HW1 create flags failed\r\n");
#if HW1_FIX_MODE == 2
    osMutexAttr_t ma = {};
    ma.name = "counter_mutex";
    ma.attr_bits = osMutexPrioInherit;
    counter_mutex = osMutexNew(&ma);
    AppCheck(counter_mutex != nullptr, "HW1 create mutex failed\r\n");
#endif
    const osThreadAttr_t a = AppThreadAttr("HW1_A", osPriorityNormal, 512U);
    const osThreadAttr_t b = AppThreadAttr("HW1_B", osPriorityNormal, 512U);
    const osThreadAttr_t c = AppThreadAttr("HW1_C", osPriorityNormal, 1024U);
    AppCheck(osThreadNew(TaskA, nullptr, &a) != nullptr, "HW1 create A failed\r\n");
    AppCheck(osThreadNew(TaskB, nullptr, &b) != nullptr, "HW1 create B failed\r\n");
    AppCheck(osThreadNew(TaskC, nullptr, &c) != nullptr, "HW1 create C failed\r\n");
    osThreadExit();
}

// 原始附件叫 Init，课件工作流叫 init。保留别名，旧工程也能连接。
// 新工程仍统一填小写 init。只让 CubeMX 创建一个初始化任务。
extern "C" void Init(void *argument) { init(argument); }
