#include "pid.h"

PID::PID(float kp, float ki, float kd,
         float dt, float outputLimit, float integralLimit)
    : kp_(kp), ki_(ki), kd_(kd),
      dt_(dt), outputLimit_(outputLimit), integralLimit_(integralLimit) {
}

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
    const float error = target - feedback;

    const float p = kp_ * error;

    // 选做题：只有关闭积分分离，或 |error| <= 阈值时，才更新积分。
    // 暂停累加时保留旧 integral_，不清零、不屏蔽，它仍然参加总输出。
    if (!integralSeparationEnabled_
        || (error >= -integralSeparationThreshold_
            && error <= integralSeparationThreshold_)) {
        integral_ += ki_ * error * dt_;
    }

    integral_ = clip(integral_, -integralLimit_, integralLimit_);

    float d = 0.0f;
    if (!firstUpdate_) {
        d = -kd_ * (feedback - lastFeedback_) / dt_;
    }

    float u = p + integral_ + d;
    u = clip(u, -outputLimit_, outputLimit_);

    lastFeedback_ = feedback;
    firstUpdate_ = false;
    return u;
}

void PID::reset() {
    // 只清除历史状态；积分分离的启用状态和阈值属于配置，必须保留。
    integral_ = 0.0f;
    lastFeedback_ = 0.0f;
    firstUpdate_ = true;
}

void PID::setIntegralSeparation(float errorThreshold) {
    integralSeparationThreshold_ = errorThreshold;
    integralSeparationEnabled_ = true;
}

void PID::disableIntegralSeparation() {
    integralSeparationEnabled_ = false;
    // 阈值可以保留，方便以后再次开启；它不影响关闭状态下的计算。
}
