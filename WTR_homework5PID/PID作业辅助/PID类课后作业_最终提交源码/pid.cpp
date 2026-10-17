#include "pid.h"

#include <cmath>

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

    const bool shouldIntegrate = !integralSeparationEnabled_
                               || std::fabs(error) <= integralSeparationThreshold_;
    if (shouldIntegrate) {
        integral_ += ki_ * error * dt_;
        integral_ = clip(integral_, -integralLimit_, integralLimit_);
    }

    float d = 0.0f;
    if (!firstUpdate_) {
        d = -kd_ * (feedback - lastFeedback_) / dt_;
    }

    const float output = clip(p + integral_ + d,
                              -outputLimit_, outputLimit_);

    lastFeedback_ = feedback;
    firstUpdate_ = false;
    return output;
}

void PID::reset() {
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
}
