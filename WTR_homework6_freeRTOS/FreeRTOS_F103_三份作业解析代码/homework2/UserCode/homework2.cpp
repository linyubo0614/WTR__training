/* 作业二：A、B 竞争同一个2000字节缓冲区。
 * 本作业用于观察问题，不给两个 memset 加锁，也不把忙等换成 osDelay。
 * 运行前将 InitTask 设成 osPriorityRealtime，A/B 是 High，C 是 Normal。
 * 改 WAIT_US 后都要重新编译、下载、复位，再记录一次不打断点的输出。
 */
#include "app_config.h"

// 按题目依次填写：100、240、300、350、500。
static constexpr uint32_t WRITE_WAIT_US = 240U;
static constexpr uint32_t BUFFER_SIZE = 2000U;

// 原附件为100ms。但115200、8N1发2000字节至少约174ms，100ms会截断。
// 默认300ms让你看到完整缓冲区；改为100可单独复现串口超时。
static constexpr uint32_t UART_TIMEOUT_MS = 300U;

// 保留原始实验中的 volatile 和 memset。它们都不是同步工具。
volatile uint8_t shared_buffer[BUFFER_SIZE];
static osEventFlagsId_t done_flags = nullptr;

// 以下变量只供 Debug Watch 阅读，不拿它们进行任务同步。
// point=0 未开始；1 A等待；2 A准备写；3 A完成；4 B准备写；5 B完成。
volatile uint32_t hw2_point = 0U;
volatile uint32_t hw2_a_begin = 0U, hw2_a_end = 0U;
volatile uint32_t hw2_b_begin = 0U, hw2_b_end = 0U;

static void BusyWaitBeforeWrite(uint32_t wait_us) {
    const uint32_t cycles_per_us = osKernelGetSysTimerFreq() / 1000000U;
    const uint32_t wait_cycles = cycles_per_us * wait_us;
    const uint32_t start = osKernelGetSysTimerCount();
    // 忙等：CPU 仍在执行 A。中断与同优先级时间片切换仍可能发生。
    // 无符号减法可处理计数器回绕，前提是等待长度小于完整计数周期。
    while (static_cast<uint32_t>(osKernelGetSysTimerCount() - start) < wait_cycles)
        __NOP();
}

static void TaskA(void *argument) {
    (void)argument;
    hw2_point = 1U; // BP1：A开始忙等。断点会改变时序。
    BusyWaitBeforeWrite(WRITE_WAIT_US);
    hw2_a_begin = osKernelGetSysTimerCount();
    hw2_point = 2U; // BP2：A即将执行 memset。
    std::memset(const_cast<uint8_t *>(shared_buffer), 'A', sizeof(shared_buffer));
    hw2_a_end = osKernelGetSysTimerCount();
    hw2_point = 3U; // BP3：A已经完成整块写入。
    AppCheck((osEventFlagsSet(done_flags, 0x01U) & osFlagsError) == 0U,
             "HW2 A flag failed\r\n");
    osThreadExit();
}

static void TaskB(void *argument) {
    (void)argument;
    hw2_b_begin = osKernelGetSysTimerCount();
    hw2_point = 4U; // BP4：B即将执行 memset。
    std::memset(const_cast<uint8_t *>(shared_buffer), 'B', sizeof(shared_buffer));
    hw2_b_end = osKernelGetSysTimerCount();
    hw2_point = 5U; // BP5：B已经完成整块写入。
    AppCheck((osEventFlagsSet(done_flags, 0x02U) & osFlagsError) == 0U,
             "HW2 B flag failed\r\n");
    osThreadExit();
}

static void TaskC(void *argument) {
    (void)argument;
    const uint32_t flags = osEventFlagsWait(done_flags, 0x03U,
                                          osFlagsWaitAll, osWaitForever);
    AppCheck((flags & osFlagsError) == 0U, "HW2 wait failed\r\n");
    // BP6：只有两个写入者都结束，C才有资格读取。
    // 事件标志保证完成时间，不保证之前写入时互斥。
    uint32_t count_a = 0U, count_b = 0U, other = 0U, runs = 0U;
    uint8_t last = 0U;
    for (uint32_t i = 0U; i < BUFFER_SIZE; ++i) {
        const uint8_t v = shared_buffer[i];
        if (v == 'A') ++count_a;
        else if (v == 'B') ++count_b;
        else ++other;
        if (i == 0U || v != last) ++runs;
        last = v;
    }
    // 先输出原实验的字符块。统计发生在全部计算结束后。
    const HAL_StatusTypeDef uart_status = HAL_UART_Transmit(&APP_UART,
        const_cast<uint8_t *>(shared_buffer), BUFFER_SIZE, UART_TIMEOUT_MS);
    char line[160] = {};
    const int n = std::snprintf(line, sizeof(line),
        "\r\nHW2 wait_us=%lu A=%lu B=%lu other=%lu runs=%lu uart_status=%u\r\n",
        static_cast<unsigned long>(WRITE_WAIT_US),
        static_cast<unsigned long>(count_a), static_cast<unsigned long>(count_b),
        static_cast<unsigned long>(other), static_cast<unsigned long>(runs),
        static_cast<unsigned>(uart_status));
    AppCheck(n > 0 && n < static_cast<int>(sizeof(line)), "HW2 format failed\r\n");
    HAL_UART_Transmit(&APP_UART, reinterpret_cast<uint8_t *>(line),
                      static_cast<uint16_t>(n), 200U);
    osThreadExit();
}

extern "C" void init(void *argument) {
    (void)argument;
    // 和附件相同：初始全是点。如果 other!=0，检查是否有人没写完。
    std::memset(const_cast<uint8_t *>(shared_buffer), '.', sizeof(shared_buffer));
    done_flags = osEventFlagsNew(nullptr);
    AppCheck(done_flags != nullptr, "HW2 create flags failed\r\n");
    const osThreadAttr_t a = AppThreadAttr("HW2_A", osPriorityHigh, 512U);
    const osThreadAttr_t b = AppThreadAttr("HW2_B", osPriorityHigh, 512U);
    const osThreadAttr_t c = AppThreadAttr("HW2_C", osPriorityNormal, 1024U);
    AppCheck(osThreadNew(TaskA, nullptr, &a) != nullptr, "HW2 create A failed\r\n");
    AppCheck(osThreadNew(TaskB, nullptr, &b) != nullptr, "HW2 create B failed\r\n");
    AppCheck(osThreadNew(TaskC, nullptr, &c) != nullptr, "HW2 create C failed\r\n");
    osThreadExit();
}
extern "C" void Init(void *argument) { init(argument); }
