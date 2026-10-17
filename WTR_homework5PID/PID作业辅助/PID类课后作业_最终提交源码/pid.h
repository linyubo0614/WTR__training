#ifndef PID_H
#define PID_H

class PID {
public:
    PID(float kp, float ki, float kd,
        float dt, float outputLimit, float integralLimit);

    float update(float target, float feedback);
    void reset();

    // Optional task: enable/disable integral separation.
    void setIntegralSeparation(float errorThreshold);
    void disableIntegralSeparation();

private:
    static float clip(float value, float minValue, float maxValue);

    // Fixed configuration.
    float kp_;
    float ki_;
    float kd_;
    float dt_;
    float outputLimit_;
    float integralLimit_;

    // Runtime history.
    float integral_ = 0.0f;
    float lastFeedback_ = 0.0f;
    bool firstUpdate_ = true;

    // Optional integral-separation configuration.
    bool integralSeparationEnabled_ = false;
    float integralSeparationThreshold_ = 0.0f;
};

#endif
