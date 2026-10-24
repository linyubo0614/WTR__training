#include "pid.h"

// 执行一次位置式PID计算
void PID_Calc(PID_t *pid)
{
    // 1. 目标减去反馈，得到当前误差
    pid->cur_error = pid->ref - pid->fdb;

    // 2. P
    float p = pid->KP * pid->cur_error;

    // 3. I
    pid->integral += pid->KI * pid->cur_error;

    // 积分限幅，避免深度饱和
    if (pid->integral > pid->integralMax)
    {
        pid->integral = pid->integralMax;
    }
    else if (pid->integral < -pid->integralMax)
    {
        pid->integral = -pid->integralMax;
    }

    // 4. D
    float d = pid->KD *(pid->cur_error - pid->last_error);

    // 5. 位置式PID：直接计算本次完整输出
    pid->output = p + pid->integral + d;

    // 6. 输出限幅
    if (pid->output > pid->outputMax)
    {
        pid->output = pid->outputMax;
    }
    else if (pid->output < -pid->outputMax)
    {
        pid->output = -pid->outputMax;
    }

    // 7. 保存误差
    pid->last_error = pid->cur_error;
}

// 清除运行状态，保留参数、目标和反馈
void PID_Reset(PID_t *pid)
{
    pid->cur_error = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    pid->output = 0.0f;
}