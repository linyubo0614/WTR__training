#ifndef PID_H
#define PID_H

// 保存PID控制器的数据
typedef struct
{
    // KI,KD包含计算周期T
    float KP;
    float KI;
    float KD;
    // 目标值reference
    float ref;

    // 实际feedback
    float fdb;

    // 当前误差
    float cur_error;

    // 上一次误差
    float last_error;

    // 积分项的累计输出（已计入积分项）
    float integral;

    // PID最终输出
    float output;

    // 输出限幅
    // 最终输出的最大绝对值
    float outputMax;

    // 积分项输出的最大绝对值
    float integralMax;

} PID_t;

// 执行一次位置式PID计算
void PID_Calc(PID_t *pid);

// 清除PID运行过程中积累的状态
void PID_Reset(PID_t *pid);

#endif