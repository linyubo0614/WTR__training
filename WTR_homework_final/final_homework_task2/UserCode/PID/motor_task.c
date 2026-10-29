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
    MOTOR_STOP,  // 没有启动资格
    MOTOR_READY, // 允许启动，等待启动
    MOTOR_RUN    // 已允许启动
} MotorState;

// 以下参数都暂定（线下调过了，但感觉效果一般）
// 这个方波跟随任务只使用了速度环
// 速度环输出电流指令
PID_t speedPID = {
    .KP = 85.0f,
    .KI = 2.0f,
    .KD = 2.0f,
    .outputMax = 4000.0f, // 输出电流限幅
    .integralMax = 1000.0f // 积分限幅
};

// ---------------------------------PID计算与将信息传入CAN ------------------------------------------
// ref 目标速度，单位rpm
static void speedServo(float ref)
{
    speedPID.ref = ref;
    speedPID.fdb = motor_data.output_speed;
    PID_Calc(&speedPID);
    CAN_SendCurrent((int16_t)speedPID.output);
}

/*------------------------------给定方波并跟随------------------------------------------*/
// 检查电调有没有正常反馈；正常就计算PID并发送电流，失联就发送零电流
void MotorTask(void *argument)
{
    (void)argument;

    // 上电先保持停止，收到有效命令后再决定运行
    MotorState run_state = MOTOR_STOP;

    // 首次启动直接可以接收命令，不用先按急停
    bool first_start = true;

    // 方波是否已经开始计时
    bool running = false;

    uint32_t start_time = 0;

    while (1)
    {
        uint32_t feedback_time = motor_data.time_ms;
        uint32_t receive_time = upper_time;
        uint32_t now = HAL_GetTick();

        // 下位机与电调连接是否正常
        bool motor_ok = motor_data.received && (uint32_t)(now - feedback_time) < 100;

        // 上位机与下位机连接是否正常
        bool upper_ok = upper_connected && (uint32_t)(now - receive_time) < UPPER_TIMEOUT_MS;

        // 确定运行状态
        if (!motor_ok || !upper_ok)
        {
            run_state = MOTOR_STOP;

            // 失联后不沿用之前收到的停止指令
            upper_stop = false;
        }
        else if (upper_stop)
        {
            run_state = MOTOR_READY;
        }
        else if (upper_start && (first_start || run_state == MOTOR_READY))
        {
            run_state = MOTOR_RUN;
            first_start = false;
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
            // 方波从200rpm开始
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