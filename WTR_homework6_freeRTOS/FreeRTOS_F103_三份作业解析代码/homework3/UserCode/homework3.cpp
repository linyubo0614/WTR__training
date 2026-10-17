/* 作业三：软件定时器 -> 两路9字节队列 -> 融合状态机 -> 串口状态机。
 * PB0按钮在原始模式与融合模式之间切换。
 * F103 RAM只有20KB，因此队列和栈使用适当的小容量。
 * 完整配置与每段代码的解释见配套文档。
 */
#include "app_config.h"
#include "fusion_core.h"

static constexpr uint32_t SENSOR1_PERIOD_MS = 100U; // 10Hz。
static constexpr uint32_t SENSOR2_PERIOD_MS = 150U; // 约6.67Hz。
static constexpr uint32_t SENSOR_QUEUE_DEPTH = 8U;
static constexpr uint32_t OUTPUT_QUEUE_DEPTH = 12U;
static constexpr uint32_t BUTTON_FLAG = 0x01U;
static constexpr uint32_t UART_DONE_FLAG = 0x01U;
static constexpr uint32_t UART_ERROR_FLAG = 0x02U;

// 设为1时，让2号的时间戳比实际时间早700ms，用来观察丢旧规则。
// 这是故障注入，不是真实采样；正常提交演示改回0。
#ifndef HW3_STALE_DEMO
#define HW3_STALE_DEMO 0
#endif
// 设为1时，将UART超时降到1ms，反复触发错误状态，LED闪烁。
// 正常运行改回0。拔掉串口线不会使TX外设自动报错。
#ifndef HW3_UART_TIMEOUT_DEMO
#define HW3_UART_TIMEOUT_DEMO 0
#endif

static osMessageQueueId_t sensor1_queue = nullptr;
static osMessageQueueId_t sensor2_queue = nullptr;
static osMessageQueueId_t output_queue = nullptr;
static osThreadId_t process_thread = nullptr;
static osThreadId_t uart_thread = nullptr;

// 只有所属任务写这些变量；它们用于调试观察，不用于跨任务同步。
volatile uint32_t g_sensor1_queue_drops = 0U;
volatile uint32_t g_sensor2_queue_drops = 0U;
volatile uint32_t g_output_queue_drops = 0U;
volatile uint32_t g_process_mode = 0U;
volatile uint32_t g_fusion_state = 0U;
volatile uint32_t g_uart_errors = 0U;
volatile uint32_t g_uart_last_status = 0U;

enum class UartState : uint32_t { WaitData = 0U, Sending = 1U, Error = 2U };
volatile uint32_t g_uart_state = 0U; // Watch中0=等待，1=发送中，2=错误。

struct SimSensor {
    uint8_t type;
    osMessageQueueId_t queue;
    uint32_t random_state; // 每个传感器各有随机种子，不竞争全局rand()。
    volatile uint32_t *drops;
};
static SimSensor sim1 = {1U, nullptr, 0x12345678U, &g_sensor1_queue_drops};
static SimSensor sim2 = {2U, nullptr, 0x87654321U, &g_sensor2_queue_drops};

// 两个 osTimerNew 都用这个回调，通过 argument 区分两个传感器。
static void SensorTimerCallback(void *argument) {
    SimSensor &s = *static_cast<SimSensor *>(argument);
    // xorshift32：生成可复现的伪随机数。不是用于安全用途的真随机数。
    uint32_t x = s.random_state;
    x ^= x << 13U; x ^= x >> 17U; x ^= x << 5U;
    s.random_state = x;
    SensorSample sample = {};
    sample.type = s.type;
    sample.value = static_cast<float>(x % 10001U) / 100.0F;
    sample.time_ms = HAL_GetTick(); // HAL用TIM2时基，1单位=1ms。
#if HW3_STALE_DEMO
    if (s.type == 2U) sample.time_ms -= 700U;
#endif
    uint8_t bytes[SENSOR_WIRE_SIZE] = {};
    EncodeSample(sample, bytes);
    // FreeRTOS的软件定时器回调在Timer服务任务中执行。
    // timeout=0：队列满马上返回，绝不阻塞整个Timer服务任务。
    // Put把9字节内容复制进队列，回调结束后局部bytes可以消失。
    if (osMessageQueuePut(s.queue, bytes, 0U, 0U) != osOK) ++(*s.drops);
}

static void PutOutput(const OutputMessage &message) {
    // 处理任务也不无限等待慢串口。满了丢当前输出并累计计数。
    if (osMessageQueuePut(output_queue, &message, 0U, 0U) != osOK)
        ++g_output_queue_drops;
}

static void ProcessTask(void *argument) {
    (void)argument;
    FusionMachine machine; // 模式和待配对数据只有这个任务拥有。
    for (;;) {
        const uint32_t button = osThreadFlagsWait(BUTTON_FLAG, osFlagsWaitAny, 0U);
        if ((button & osFlagsError) == 0U && (button & BUTTON_FLAG) != 0U) {
            machine.ToggleMode();
            // 清掉模式切换前排队的样本与输出，从新数据重新开始。
            AppCheck(osMessageQueueReset(sensor1_queue) == osOK, "Reset S1 failed\r\n");
            AppCheck(osMessageQueueReset(sensor2_queue) == osOK, "Reset S2 failed\r\n");
            AppCheck(osMessageQueueReset(output_queue) == osOK, "Reset TX failed\r\n");
            OutputMessage note = {};
            note.kind = OutputKind::ModeChanged;
            note.mode = machine.mode;
            PutOutput(note);
            // 正在UART硬件里发送的上一帧允许结束，不强行切掉半帧。
        }
        uint8_t bytes[SENSOR_WIRE_SIZE] = {};
        OutputMessage output = {};
        // 融合时一个槽有数据就暂不再取同路数据，避免反复覆盖待配对样本。
        // 另一路没数据时也不能无限阻塞，否则按钮切换无法及时处理。
        if (machine.CanAccept(1U) &&
            osMessageQueueGet(sensor1_queue, bytes, nullptr, 0U) == osOK) {
            if (machine.Accept(DecodeSample(bytes), output)) PutOutput(output);
        }
        if (machine.CanAccept(2U) &&
            osMessageQueueGet(sensor2_queue, bytes, nullptr, 0U) == osOK) {
            if (machine.Accept(DecodeSample(bytes), output)) PutOutput(output);
        }
        if (machine.Step(output)) PutOutput(output);
        g_process_mode = static_cast<uint32_t>(machine.mode);
        g_fusion_state = static_cast<uint32_t>(machine.state);
        osDelay(AppMsToTicks(5U)); // 给其他任务和Idle留执行机会。
    }
}

static void ButtonTask(void *argument) {
    (void)argument;
    GPIO_PinState stable = HAL_GPIO_ReadPin(APP_BUTTON_PORT, APP_BUTTON_PIN);
    GPIO_PinState candidate = stable;
    uint32_t candidate_since = HAL_GetTick();
    for (;;) {
        const GPIO_PinState now = HAL_GPIO_ReadPin(APP_BUTTON_PORT, APP_BUTTON_PIN);
        if (now != candidate) {
            candidate = now;
            candidate_since = HAL_GetTick();
        }
        // 同一电平保持至少30ms才承认，按下和松开都消抖。
        if (candidate != stable && HAL_GetTick() - candidate_since >= 30U) {
            stable = candidate;
            if (stable == APP_BUTTON_PRESSED) {
                // 一个稳定的按下边沿只通知一次，按住不反复切换。
                const uint32_t result = osThreadFlagsSet(process_thread, BUTTON_FLAG);
                AppCheck((result & osFlagsError) == 0U, "Button notify failed\r\n");
            }
        }
        osDelay(AppMsToTicks(10U));
    }
}

// 避免printf浮点选项与大体积的浮点格式化库，自己转成两位小数字符串。
// 本例测量范围只有0..100，乘100后不会溢出int32_t。
static void ValueText(float value, char *text, uint32_t size) {
    const int32_t scaled = static_cast<int32_t>(value * 100.0F +
                                               (value >= 0.0F ? 0.5F : -0.5F));
    const uint32_t magnitude = static_cast<uint32_t>(scaled < 0 ? -scaled : scaled);
    std::snprintf(text, size, "%s%lu.%02lu", scaled < 0 ? "-" : "",
        static_cast<unsigned long>(magnitude / 100U),
        static_cast<unsigned long>(magnitude % 100U));
}

static int FormatOutput(const OutputMessage &msg, char *line, uint32_t size) {
    char value[20] = {};
    ValueText(msg.value, value, sizeof(value));
    switch (msg.kind) {
    case OutputKind::Raw:
        return std::snprintf(line, size,
            "UART event=SENDING RAW sensor=%u value=%s time_ms=%lu\r\n",
            static_cast<unsigned>(msg.sensor_type), value,
            static_cast<unsigned long>(msg.time1_ms));
    case OutputKind::Fused:
        return std::snprintf(line, size,
            "UART event=SENDING FUSED mean=%s t1=%lu t2=%lu gap_ms=%lu\r\n",
            value, static_cast<unsigned long>(msg.time1_ms),
            static_cast<unsigned long>(msg.time2_ms),
            static_cast<unsigned long>(msg.gap_ms));
    case OutputKind::DropOld:
        return std::snprintf(line, size,
            "UART event=SENDING DROP_OLD sensor=%u t1=%lu t2=%lu gap_ms=%lu\r\n",
            static_cast<unsigned>(msg.sensor_type),
            static_cast<unsigned long>(msg.time1_ms),
            static_cast<unsigned long>(msg.time2_ms),
            static_cast<unsigned long>(msg.gap_ms));
    case OutputKind::ModeChanged:
        return std::snprintf(line, size, "UART event=SENDING MODE=%s\r\n",
                            msg.mode == ProcessMode::Raw ? "RAW" : "FUSED");
    }
    return -1;
}

static void UartTask(void *argument) {
    (void)argument;
    UartState state = UartState::WaitData;
    OutputMessage message = {};
    // line在这个永不结束的任务栈上，发送完成前绝不覆盖。
    // IT发送只保存指针，不复制整条字符串，因此不能用临时局部数组。
    char line[176] = {};
    int length = 0;
    bool started = false;
    bool first_wait_notice = true;
    for (;;) {
        g_uart_state = static_cast<uint32_t>(state);
        switch (state) {
        case UartState::WaitData: {
            osStatus_t result = osErrorTimeout;
            if (!first_wait_notice)
                result = osMessageQueueGet(output_queue, &message, nullptr,
                                           AppMsToTicks(1000U));
            if (first_wait_notice || result == osErrorTimeout) {
                // event表示刚才的状态。状态报告本身也需要短暂占用UART。
                length = std::snprintf(line, sizeof(line), "UART event=WAIT_DATA\r\n");
                first_wait_notice = false;
            } else if (result == osOK) {
                length = FormatOutput(message, line, sizeof(line));
            } else {
                g_uart_last_status = 0x100U;
                state = UartState::Error;
                break;
            }
            if (length <= 0 || length >= static_cast<int>(sizeof(line))) {
                g_uart_last_status = 0x101U;
                state = UartState::Error;
            } else {
                started = false;
                state = UartState::Sending;
            }
            break;
        }
        case UartState::Sending: {
            if (!started) {
                // 清上一帧遗留的标志，必须在启动发送之前清。
                osThreadFlagsClear(UART_DONE_FLAG | UART_ERROR_FLAG);
                const HAL_StatusTypeDef result = HAL_UART_Transmit_IT(&APP_UART,
                    reinterpret_cast<uint8_t *>(line), static_cast<uint16_t>(length));
                if (result != HAL_OK) {
                    g_uart_last_status = static_cast<uint32_t>(result);
                    state = UartState::Error;
                    break;
                }
                started = true;
            }
            // 本任务阻塞等完成通知，CPU能去做采样、融合和按键。
            // 176字节在115200下约15.3ms，200ms留有充足余量。
            const uint32_t timeout_ms = HW3_UART_TIMEOUT_DEMO ? 1U : 200U;
            const uint32_t flags = osThreadFlagsWait(UART_DONE_FLAG | UART_ERROR_FLAG,
                osFlagsWaitAny, AppMsToTicks(timeout_ms));
            if ((flags & osFlagsError) != 0U || (flags & UART_ERROR_FLAG) != 0U) {
                g_uart_last_status = (flags & osFlagsError) != 0U ? flags : 0x102U;
                state = UartState::Error;
            } else if ((flags & UART_DONE_FLAG) != 0U) {
                started = false;
                HAL_GPIO_WritePin(APP_LED_PORT, APP_LED_PIN, GPIO_PIN_SET);
                state = UartState::WaitData;
            }
            break;
        }
        case UartState::Error:
            ++g_uart_errors;
            // 没有DMA，同步AbortTransmit会关掉TX中断并停止使用line。
            // 只有确认旧传输已停止，下面才能重写缓冲区。
            AppCheck(HAL_UART_AbortTransmit(&APP_UART) == HAL_OK,
                     "UART abort failed\r\n");
            osThreadFlagsClear(UART_DONE_FLAG | UART_ERROR_FLAG);
            started = false;
            HAL_GPIO_TogglePin(APP_LED_PORT, APP_LED_PIN); // 通信坏了仍能看到错误。
            osDelay(AppMsToTicks(200U)); // 防止错误循环耗尽CPU。
            // 尝试恢复通道并发出错误记录。故障中的消息会丢弃，不无限重发。
            length = std::snprintf(line, sizeof(line),
                "UART event=ERROR count=%lu reason=%lu retry\r\n",
                static_cast<unsigned long>(g_uart_errors),
                static_cast<unsigned long>(g_uart_last_status));
            state = UartState::Sending;
            break;
        }
    }
}

// 这些函数由C编译的HAL驱动调用，所以必须用extern "C"。
// USART1 NVIC设为5，不能设0..4后还在回调里调用RTOS。
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &APP_UART && uart_thread != nullptr)
        osThreadFlagsSet(uart_thread, UART_DONE_FLAG);
    // 硬件中断中只发通知，不串口打印、不延时、不等待队列。
}
extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart == &APP_UART && uart_thread != nullptr)
        osThreadFlagsSet(uart_thread, UART_ERROR_FLAG);
}

extern "C" void init(void *argument) {
    (void)argument;
    sensor1_queue = osMessageQueueNew(SENSOR_QUEUE_DEPTH, SENSOR_WIRE_SIZE, nullptr);
    sensor2_queue = osMessageQueueNew(SENSOR_QUEUE_DEPTH, SENSOR_WIRE_SIZE, nullptr);
    output_queue = osMessageQueueNew(OUTPUT_QUEUE_DEPTH, sizeof(OutputMessage), nullptr);
    AppCheck(sensor1_queue && sensor2_queue && output_queue, "HW3 create queues failed\r\n");
    sim1.queue = sensor1_queue;
    sim2.queue = sensor2_queue;

    const osThreadAttr_t proc = AppThreadAttr("HW3_Process", osPriorityAboveNormal, 1024U);
    const osThreadAttr_t uart = AppThreadAttr("HW3_UART", osPriorityNormal, 1536U);
    const osThreadAttr_t button = AppThreadAttr("HW3_Button", osPriorityNormal, 512U);
    process_thread = osThreadNew(ProcessTask, nullptr, &proc);
    uart_thread = osThreadNew(UartTask, nullptr, &uart);
    AppCheck(process_thread && uart_thread, "HW3 create tasks failed\r\n");
    AppCheck(osThreadNew(ButtonTask, nullptr, &button) != nullptr,
             "HW3 create button failed\r\n");

    const osTimerId_t timer1 = osTimerNew(SensorTimerCallback, osTimerPeriodic, &sim1, nullptr);
    const osTimerId_t timer2 = osTimerNew(SensorTimerCallback, osTimerPeriodic, &sim2, nullptr);
    AppCheck(timer1 && timer2, "HW3 create timers failed\r\n");
    AppCheck(osTimerStart(timer1, AppMsToTicks(SENSOR1_PERIOD_MS)) == osOK,
             "HW3 start timer1 failed\r\n");
    AppCheck(osTimerStart(timer2, AppMsToTicks(SENSOR2_PERIOD_MS)) == osOK,
             "HW3 start timer2 failed\r\n");
    osThreadExit(); // InitTask优先级Realtime，所有资源建好后才放行其他任务。
}
extern "C" void Init(void *argument) { init(argument); }
