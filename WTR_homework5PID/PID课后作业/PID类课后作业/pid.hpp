#ifndef PID_H
#define PID_H

// 本作业采用并联形式：u = P + I + D。
// error = target - feedback；
// 为说明单位，记：Y = 被控量单位，U = 控制输出单位，时间单位为秒。
// 例如控制速度时，Y 可以是 rad/s；U 取决于执行器的控制接口。
class PID {
public:
    /*
     * 构造函数：创建一个 PID 对象，并设置它自己的参数。
     * kp            比例增益，单位 U/Y；可为 0，关闭比例项。
     * ki            积分增益，单位 U/(Y*s)；可为 0，关闭积分累加。
     * kd            微分增益，单位 U*s/Y；可为 0，关闭微分项。
     * dt            相邻两次 update() 之间的固定时间间隔，单位秒，必须 > 0。
     * outputLimit   总输出的绝对值上限，单位 U，必须 > 0。
     *               例如传入 100，最终输出范围为 [-100, +100]。
     * integralLimit 积分输出 I 的绝对值上限，单位 U，必须 > 0。
     *               限制的是已乘 ki 的积分状态，不是原始误差和。
     *
     * 本作业假设所有参数与输入均为有效有限数值，不要求实现异常处理。
     * kp、ki、kd 取非负值，控制方向按“正输出使反馈增大”理解。
     */
    PID(float kp, float ki, float kd,
        float dt, float outputLimit, float integralLimit);

    /*
     * 执行一个采样周期的 PID 计算。
     * target   本次目标值，单位 Y。
     * feedback 本次测量反馈值，单位 Y，与 target 使用相同单位。
     * 返回值   最终控制输出 u，单位 U，已进行总输出限幅。
     *
     * 调用一次表示时间前进 dt 秒。
     * 第一次调用、reset() 后第一次调用，微分项 D 必须取 0。
     * 此函数必须更新并保存积分状态与本次反馈值，供下次调用使用。
     */
    float update(float target, float feedback);

    /*
     * 清除控制器的历史状态。
     * 将积分状态、历史反馈清零，并恢复“第一次调用”标记。
     * 保留 kp、ki、kd、dt 及两个限幅参数，不修改控制器配置。
     */
    void reset();

    //积分分离
    void setIntegralSeparation(float errorThreshold); // 开启，阈值 >= 0，单位 Y
    void disableIntegralSeparation();                 // 关闭，恢复通常的积分累加

private:
    // 范围截断工具，已经提供实现，不作为本次考核重点。
    // value：待截断值；minValue/maxValue：下界和上界，要求下界 <= 上界。
    static float clip(float value, float minValue, float maxValue);

    // ---------- 配置参数：本作业中创建对象后保持不变 ----------
    float kp_;             // 比例增益，单位 U/Y。
    float ki_;             // 积分增益，单位 U/(Y*s)。
    float kd_;             // 微分增益，单位 U*s/Y。
    float dt_;             // 固定采样周期，单位秒。
    float outputLimit_;    // 最终输出 u 的绝对值上限，单位 U。
    float integralLimit_;  // 保存的积分输出 I 的绝对值上限，单位 U。

    // ---------- 历史状态：属于当前对象，必须跨调用保存 ----------
    float integral_ = 0.0f;      // 历次 ki * error * dt 的累积结果，已乘 ki。
    float lastFeedback_ = 0.0f;  // 上一次 update() 的反馈值，单位 Y。
    bool firstUpdate_ = true;   // true 表示没有可用的上一次反馈，首次 D=0。

    //积分分离
    bool integralSeparationEnabled_ = false;
    float integralSeparationThreshold_ = 0.0f;
};

#endif
