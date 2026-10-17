#include "pid.h"

// 构造函数：用初始化列表把传入参数保存到当前对象的成员变量中。
PID::PID(float kp, float ki, float kd,
         float dt, float outputLimit, float integralLimit)
    : kp_(kp), ki_(ki), kd_(kd),
      dt_(dt), outputLimit_(outputLimit), integralLimit_(integralLimit) {
    // 历史状态 integral_、lastFeedback_、firstUpdate_ 已在 pid.h 中给定初值。
}

// 将 value 截断到 [minValue, maxValue]。
float PID::clip(float value, float minValue, float maxValue) {
    if (value > maxValue) {
        return maxValue;
    }
    if (value < minValue) {
        return minValue;
    }
    return value;
}

float PID::update(float target, float feedback) {
    // 1. 误差方向：目标值 - 反馈值。
    const float error = target - feedback;

    // 2. 比例项 P = Kp * e。
    const float p = kp_ * error;

    // 3. 积分项 I 累加：I += Ki * e * dt。
    //    注意：integral_ 保存的是已经乘过 Ki 的“积分输出”，合成时直接加。
    integral_ += ki_ * error * dt_;

    // 4. 对积分状态本身限幅，并把限幅后的值写回 integral_。
    integral_ = clip(integral_, -integralLimit_, integralLimit_);

    // 5. 微分项 D。首次调用没有上一次反馈，因此 D=0。
    //    采用“测量值微分”：D = -Kd * (feedback - lastFeedback) / dt。
    float d = 0.0f;
    if (!firstUpdate_) {
        d = -kd_ * (feedback - lastFeedback_) / dt_;
    }

    // 6. 合成总输出，并做输出限幅。
    float u = p + integral_ + d;
    u = clip(u, -outputLimit_, outputLimit_);

    // 7. 本次微分计算完成后再保存本次 feedback，并取消首次标记。
    lastFeedback_ = feedback;
    firstUpdate_ = false;

    // 8. 返回限幅后的最终输出。
    return u;
}

void PID::reset() {
    // 只清除历史状态，配置参数 kp_/ki_/kd_/dt_/outputLimit_/integralLimit_ 保持不变。
    integral_ = 0.0f;
    lastFeedback_ = 0.0f;
    firstUpdate_ = true;
}
