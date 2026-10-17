#ifndef PID_H
#define PID_H

// 本作业采用并联形式：u = P + I + D。
// error = target - feedback。
// 基础版接口保持不变；下面额外增加了选做题“积分分离”的接口。
class PID {
public:
    PID(float kp, float ki, float kd,
        float dt, float outputLimit, float integralLimit);

    float update(float target, float feedback);
    void reset();

    // ---------- 选做题：积分分离 ----------
    // 开启积分分离：|error| <= errorThreshold 时才累加积分。
    // errorThreshold 的单位与被控量 Y 相同，调用者应传入 >= 0 的值。
    void setIntegralSeparation(float errorThreshold);

    // 关闭积分分离，恢复“每次 update 都累加积分”的基础行为。
    void disableIntegralSeparation();

private:
    static float clip(float value, float minValue, float maxValue);

    // ---------- 配置参数 ----------
    float kp_;
    float ki_;
    float kd_;
    float dt_;
    float outputLimit_;
    float integralLimit_;

    // ---------- 历史状态 ----------
    float integral_ = 0.0f;
    float lastFeedback_ = 0.0f;
    bool firstUpdate_ = true;

    // ---------- 积分分离配置：属于控制器配置，reset() 不应清除 ----------
    bool integralSeparationEnabled_ = false;
    float integralSeparationThreshold_ = 0.0f;
};

#endif
