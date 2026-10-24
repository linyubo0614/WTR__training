#include "motor_task.h"
#include "pid.h"
#include "CANCommunication.h"
#include "main.h"
#include "cmsis_os2.h"
#include "uart_task.h"
#include <math.h>

// 初始反馈状态
volatile uint8_t motor_state = 0xFF;

// 电机运行状态枚举
typedef enum
{
    MOTOR_STOP,
    MOTOR_READY,
    MOTOR_RUN
} MotorState;

// 速度环PID（具体参数还得调）
PID_t speedPID =
    {
        .KP = 10.0f,
        .KI = 0.01f,
        .KD = 0.0f,

        // 作业设定的4000上限
        .outputMax = 4000.0f,

        // 内环使用I，同时加积分限幅
        .integralMax = 1000.0f};

/*------------------------给定目标后计算 速度PID（内环） 并且控制电调-------------------------------*/
// ref为目标速度，位置式pid速度闭环实现任务2的给定方波自动跟随
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

/*------------------------------给定方波并跟随------------------------------------------*/
// 检查电调有没有正常反馈；正常就计算PID并发送电流，失联就发送零电流
void MotorTask(void *argument)
{
    (void)argument;

    // 上电停止，等待停止指令和启动请求
    MotorState run_state = MOTOR_STOP;

    // 方波是否已经开始计时
    bool running = false;

    uint32_t start_time = 0;

    while (1)
    {
        uint32_t feedback_time = motor_data.time_ms;
        uint32_t receive_time = host_time;
        uint32_t now = HAL_GetTick();

        // 电机反馈是否正常
        bool motor_ok =
            motor_data.received &&
            (uint32_t)(now - feedback_time) < 100;

        // 上位机连接是否正常
        bool host_ok =
            host_connected &&
            (uint32_t)(now - receive_time) < HOST_TIMEOUT_MS;

        // 确定运行状态
        if (!motor_ok || !host_ok)
        {
            run_state = MOTOR_STOP;

            // 失联后不沿用之前收到的停止指令
            host_stop = false;
        }
        else if (host_stop)
        {
            run_state = MOTOR_READY;
        }
        else if (run_state == MOTOR_READY)
        {
            run_state = MOTOR_RUN;
        }

        if (run_state != MOTOR_RUN)
        {
            PID_Reset(&speedPID);
            CAN_SendCurrent(0);

            running = false;

            if (!motor_ok)
            {
                // CAN反馈异常
                motor_state = 0xFF;
            }
            else
            {
                // 速度模式，不报告目标已经到达
                motor_state = 0xAA;
            }
        }
        else
        {
            // 每次启动，方波从200rpm开始
            if (!running)
            {
                start_time = now;
                running = true;
            }

            uint32_t seconds = (now - start_time) / 1000;

            if (seconds % 2 == 0)
            {
                speedServo(200.0f);
            }
            else
            {
                speedServo(0.0f);
            }

            // 判断实际速度是否接近当前目标速度
            if (fabsf(speedPID.ref - motor_data.output_speed) < 5.0f)
            {
                // 速度模式，已经到达目标
                motor_state = 0xAF;
            }
            else
            {
                // 速度模式，正在接近目标
                motor_state = 0xAA;
            }
        }

        osDelay(2);
    }
}