#include "motor_task.h"
#include "pid.h"
#include "CANCommunication.h"
#include "main.h"
#include "cmsis_os2.h"

// 速度环PID（具体参数还得调）
PID_t speedPID =
{
    .KP = 10.0f,
    .KI = 0.01f,
    .KD = 0.0f,

    // 作业设定的4000上限
    .outputMax = 4000.0f,

    // 内环使用I，同时加积分限幅
    .integralMax = 1000.0f
};

// 位置环PID（具体参数还得调）
static PID_t posisionPID =
 {
    .KP = 2.0f,
    .KI = 0.0f,
    .KD = 0.0f,

    // 目标转速最大绝对值，单位 rpm
    .outputMax = 120.0f,

    // 外环先不使用积分
    .integralMax = 0.0f
};

/*------------------------给定目标后计算 速度PID（内环） 并且控制电调-------------------------------*/
// ref反馈输入，位置式pid速度闭环实现任务2的给定方波自动跟随
static void speedServo(float ref)
{
    // 目标速度
    speedPID.ref = ref;

    // 实际输出轴速度
    speedPID.fdb = motor_data.output_speed;

    // 执行一次位置式PID计算
    PID_Calc(&speedPID);

    // PID输出电流指令发送给电调
    CAN_SendCurrent((int16_t)speedPID.output);
}

/*-------------------------给定目标后计算 位置PID（外环） 并且控制电调-------------------------------*/
/**
 * ref 为目标角度
 * max_speed 为最大转速
 *  */ 
static void positionServo(float ref, float max_speed)
{
    // 目标位置
    posisionPID.ref = ref;

    // 实际位置
    posisionPID.fdb = motor_data.position;

    // 输出限幅
    posisionPID.outputMax = max_speed;

    // 执行一次位置式PID计算
    PID_Calc(&posisionPID);

    // 将目标转速交给速度环（外环）
    speedServo(posisionPID.output);
}

/*------------------------------给定方波并跟随------------------------------------------*/
// 检查电调有没有正常反馈；正常就计算PID并发送电流，失联就发送零电流。按1ms周期重复这个过程。
void MotorTask(void *argument)
{
    (void)argument;

    // 是否开始方波
    bool running = false;

    // 方波开始运行的时间ms
    uint32_t start_time = 0;

    while (1)
    {
        uint32_t now = HAL_GetTick();

        // 失联情况
        // 未收到过反馈，或超过100ms没有再次反馈
        if (!motor_data.received ||(uint32_t)(now - motor_data.time_ms) >= 100)
        {
            // 清除历史积分和误差
            PID_Reset(&speedPID);

            // 失联电流发送0
            CAN_SendCurrent(0);

            running = false;
        }
        else
        {
            // 第一次启动/中断后重新恢复正常时，从200rpm开始
            if (running==false)
            {
                start_time = now;
                running = true;
            }

            // 计算已经经过了多少个完整的1秒
            uint32_t seconds = (now - start_time) / 1000;

            // 第偶数秒内目标200rpm
            if (seconds % 2 == 0)
            {
                speedServo(200.0f);
            }
            // 第奇数秒内目标0rpm
            else
            {
                speedServo(0.0f);
            }
        }

        osDelay(2);
    }
}