#include "pid.hpp"
#include <cmath>

// 构造函数已经提供。冒号后的列表给当前对象的成员变量赋初值。
// 左侧 kp_ 是成员变量，括号中的 kp 是传入的构造参数，以此类推。
PID::PID(float kp, float ki, float kd,
         float dt, float outputLimit, float integralLimit)
    : kp_(kp), ki_(ki), kd_(kd),
      dt_(dt), outputLimit_(outputLimit), integralLimit_(integralLimit) {
    // 历史状态在 pid.h 中已经给定初值。
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
    // TODO 1：计算本次误差 error，方向为“目标 - 反馈”。
    const float error = target - feedback;

    // TODO 2：根据本次误差计算比例项 P。
    const float p = kp_ * error;

    // TODO 3：更新 integral_。本作业显式使用 dt_，不可漏掉采样周期。
    // integral_ 已经是积分输出 I，合成总输出时不要再乘一次 ki_。
    if (!integralSeparationEnabled_|| std::fabs(error) <= integralSeparationThreshold_) 
    {
        integral_ += ki_ * error * dt_;
    }


    // TODO 4：对 integral_ 本身限幅，并将限幅后的结果写回 integral_。
    // 使用 clamp()，范围为 [-integralLimit_, +integralLimit_]。
   
    integral_ = clip(integral_, -integralLimit_, integralLimit_);

    // TODO 5：计算微分项 D。
    // 首次计算时取 0；其他时候根据反馈变化率计算，注意负号和 dt_。
    float d = 0.0f;
    if (!firstUpdate_) {
        d = -kd_ * (feedback - lastFeedback_) / dt_;
    }

    // TODO 6：合成 P、I、D，再对最终输出进行 outputLimit_ 限幅。
    float output=p+integral_+d;
    output = clip(output, -outputLimit_, outputLimit_);
     
    // TODO 7：保存本次 feedback，并更新首次计算标记。
    // 历史反馈应在本次微分计算之后更新。
    lastFeedback_ = feedback;
    firstUpdate_ = false;

    // TODO 8：返回本次最终控制输出。
    return output;
}

//开启积分限幅
void PID::setIntegralSeparation(float errorThreshold) {
    integralSeparationThreshold_ = errorThreshold;
    integralSeparationEnabled_ = true;
}

//关闭积分限幅
void PID::disableIntegralSeparation() {
    integralSeparationEnabled_ = false;
}

void PID::reset() {
    // TODO：清除三个历史状态，保留所有配置参数。
    integral_ = 0.0f;
    lastFeedback_ = 0.0f;
    firstUpdate_= true;
}
