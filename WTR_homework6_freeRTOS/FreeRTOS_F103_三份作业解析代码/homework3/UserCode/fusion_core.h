#pragma once
// 这个文件只处理数据，不依赖 STM32、HAL 或 RTOS。
// 可以单独在电脑上测试；板上的 ProcessTask 也使用同一份逻辑。
#include <cstdint>
#include <cstring>

static_assert(sizeof(float) == 4U, "This homework needs a 4-byte float");
static constexpr uint32_t SENSOR_WIRE_SIZE = 9U;

// 这个结构体便于计算，编译器通常把它对齐到12字节。
// 队列里的格式必须是9字节，因此不能直接把 sizeof(SensorSample) 当9。
struct SensorSample {
    uint8_t type;     // 1=传感器1，2=传感器2。
    float value;      // 本例模拟0.00到100.00的测量值。
    uint32_t time_ms; // HAL_GetTick()，单位毫秒，32位回绕。
};

inline void EncodeSample(const SensorSample &s, uint8_t out[SENSOR_WIRE_SIZE]) {
    out[0] = s.type;
    std::memcpy(out + 1U, &s.value, 4U);
    std::memcpy(out + 5U, &s.time_ms, 4U);
    // memcpy 避免从未对齐地址直接访问 float/uint32_t。
    // 本例在同一块F103上收发，采用芯片本机的小端表示。
}

inline SensorSample DecodeSample(const uint8_t in[SENSOR_WIRE_SIZE]) {
    SensorSample s = {};
    s.type = in[0];
    std::memcpy(&s.value, in + 1U, 4U);
    std::memcpy(&s.time_ms, in + 5U, 4U);
    return s;
}

enum class ProcessMode : uint8_t { Raw = 0U, Fused = 1U };
enum class FusionState : uint8_t {
    WaitBoth,   // 两个传感器都还没有待配对数据。
    WaitFirst,  // 已有2号数据，等1号。
    WaitSecond, // 已有1号数据，等2号。
    CheckPair   // 两个都有，比较时间、融合或丢弃旧数据。
};
enum class OutputKind : uint8_t { Raw, Fused, DropOld, ModeChanged };

// 这是处理任务到串口任务的内部消息，不受题目的9字节传感器格式限制。
struct OutputMessage {
    OutputKind kind;
    ProcessMode mode;
    uint8_t sensor_type;
    float value;
    uint32_t time1_ms;
    uint32_t time2_ms;
    uint32_t gap_ms;
};

// 本例仅比较时间相邻的数据，时间差必须小于2^31ms（约24.8天）。
// unsigned 减法能处理计数回绕，不要直接用 abs(a-b)。
inline uint32_t TimeGapMs(uint32_t a, uint32_t b) {
    const uint32_t forward = a - b;
    return forward <= 0x7FFFFFFFU ? forward : b - a;
}
inline bool FirstIsOlder(uint32_t a, uint32_t b) {
    return (a - b) > 0x7FFFFFFFU;
}

class FusionMachine {
public:
    ProcessMode mode = ProcessMode::Raw; // 上电先分别输出两路原始值。
    FusionState state = FusionState::WaitBoth;

    void ToggleMode() {
        mode = mode == ProcessMode::Raw ? ProcessMode::Fused : ProcessMode::Raw;
        have_first_ = false;
        have_second_ = false;
        state = FusionState::WaitBoth; // 不让切换前的半组数据参与下一模式。
    }

    bool CanAccept(uint8_t type) const {
        if (type != 1U && type != 2U) return false;
        if (mode == ProcessMode::Raw) return true;
        return type == 1U ? !have_first_ : !have_second_;
    }

    // 有输出返回true。原始模式马上输出；融合模式先存一份等另一份。
    bool Accept(const SensorSample &sample, OutputMessage &out) {
        if (!CanAccept(sample.type)) return false;
        if (mode == ProcessMode::Raw) {
            out = {};
            out.kind = OutputKind::Raw;
            out.mode = mode;
            out.sensor_type = sample.type;
            out.value = sample.value;
            out.time1_ms = sample.time_ms;
            return true;
        }
        if (sample.type == 1U) { first_ = sample; have_first_ = true; }
        else { second_ = sample; have_second_ = true; }
        UpdateWaitState();
        return false;
    }

    // 显式状态机。每轮最多处理一对，避免一个任务一直占CPU。
    bool Step(OutputMessage &out) {
        if (mode == ProcessMode::Raw) return false;
        switch (state) {
        case FusionState::WaitBoth:
        case FusionState::WaitFirst:
        case FusionState::WaitSecond:
            return false;
        case FusionState::CheckPair:
            out = {};
            out.mode = mode;
            out.time1_ms = first_.time_ms;
            out.time2_ms = second_.time_ms;
            out.gap_ms = TimeGapMs(first_.time_ms, second_.time_ms);
            if (out.gap_ms > 500U) { // 注意等于500ms仍允许融合。
                out.kind = OutputKind::DropOld;
                if (FirstIsOlder(first_.time_ms, second_.time_ms)) {
                    out.sensor_type = 1U;
                    have_first_ = false; // 丢1号旧数据，保留2号新数据。
                } else {
                    out.sensor_type = 2U;
                    have_second_ = false;
                }
            } else {
                out.kind = OutputKind::Fused;
                out.value = (first_.value + second_.value) * 0.5F;
                // 这里是两路同类测量值的等权均值，不是历史滑动平均。
                have_first_ = false;
                have_second_ = false; // 每份数据只用于一次融合。
            }
            UpdateWaitState();
            return true; // 丢弃事件也发给串口，方便观察500ms规则。
        }
        return false;
    }

private:
    SensorSample first_ = {}, second_ = {};
    bool have_first_ = false, have_second_ = false;
    void UpdateWaitState() {
        if (have_first_ && have_second_) state = FusionState::CheckPair;
        else if (have_first_) state = FusionState::WaitSecond;
        else if (have_second_) state = FusionState::WaitFirst;
        else state = FusionState::WaitBoth;
    }
};
