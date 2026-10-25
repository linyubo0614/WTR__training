#include "motor_task.h"
#include "pid.h"
#include "action.h"
#include "CANCommunication.h"
#include "uart_task.h"
#include "main.h"
#include "cmsis_os2.h"
#include <math.h>

typedef enum
{
    MOTOR_STOP,  // 没有启动资格
    MOTOR_READY, // 允许启动，等待启动
    MOTOR_RUN    // 已允许启动
} MotorState;

// 上电先保持不允许启动，收到有效命令再进入ready
static volatile MotorState run_state = MOTOR_STOP;

// 首次启动直接可以接收命令，不用先按急停
static bool first_start = true;

/**
 * motor_state 是协议规定的反馈状态
 * 0xAA 速度模式 正在前往目标速度
 * 0xAF 速度模式 到达目标速度
 * 0xBA 位置控制 正在前往目标位置
 * 0xBF 位置控制 到达目标位置
 * 0xFF 异常
 */
volatile uint8_t motor_state = 0xFF;

// 以下参数都暂定
// 速度环输出电流指令
PID_t speedPID = {
    .KP = 10.0f,
    .KI = 0.01f,
    .KD = 0.0f,
    .outputMax = MOTOR_CURRENT_LIMIT, // 输出电流限幅
    .integralMax = 1000.0f            // 积分限幅
};

// 位置环输出给位置环作为输出轴目标转速
PID_t positionPID = {
    .KP = 2.0f,
    .KI = 0.0f,
    .KD = 0.0f,
    .outputMax = 120.0f, // 输出转速限幅
    .integralMax = 20.0f // 积分限幅（外环先不用I）
};

// ---------------------------------串级PID计算与将信息传入CAN ------------------------------------------
/**
 * 内环速度环，外环位置环
 *
 * 我把任务2的“在速度闭环的基础上通过串级 PID 构成位置闭环。输入为 角度 deg”
 * 和任务1，3的要求合并了
 * 另外一个task2的文件夹中的pid只是单环PID，用于实现给定方波跟随
 */
// ref 目标速度，单位rpm
static void speedServo(float ref)
{
    speedPID.ref = ref;
    speedPID.fdb = motor_data.output_speed;
    PID_Calc(&speedPID);
    CAN_SendCurrent((int16_t)speedPID.output);
}

// ref 目标位置（角度），单位deg
static void positionServo(float ref, float max_speed)
{
    positionPID.ref = ref;
    positionPID.fdb = motor_data.position;
    positionPID.outputMax = max_speed;
    PID_Calc(&positionPID);
    speedServo(positionPID.output);
}

// ----------------------------------获取指令 调度状态--------------------------------
/**
 * 本函数接收来自串口的指令
 * 用于调度状态
 */
// 收到的指令，来自串口（在UartTask中调用）
static volatile Command motor_command = {0};
// 指令id，每次接收到命令后用于判断是否切换到新的运行状态
// 如果切换或者断联后重新恢复连接 需要重置PID，否则不用管
static volatile uint32_t command_id = 0;

void Motor_SetCommand(const Command *command)
{
    int32_t lock_state = osKernelLock();

    // 上次收到CAN反馈的时间
    uint32_t feedback_time = motor_data.time_ms;
    uint32_t now = HAL_GetTick();

    // 是否收到过电调反馈，并且最近100 ms内仍有反馈（用于判断电调和下位机是否正常连通）
    bool motor_ok = motor_data.received && (uint32_t)(now - feedback_time) < 100;

    // 必须上下位机连接正常，下位机与电调连接顺畅才判断，否则判断为无启动条件
    if (!motor_ok || !upper_connected || (uint32_t)(now - upper_time) >= UPPER_TIMEOUT_MS)
        run_state = MOTOR_STOP;

    if (motor_ok)
    {
        // 如果上位机急停（切换为速度模式，且最大速度限制为0）
        // 切换为停止但是允许启动模式
        if (command->mode == 0x00 && command->a == 0)
            run_state = MOTOR_READY;

        // 如果是允许启动状态但是还暂停，切换为运行状态
        else if (first_start || run_state == MOTOR_READY)
        {
            run_state = MOTOR_RUN;
            first_start = false;
        }

    }

    
    // 判断是否发生改变，如果是定时发送相同信息则可能不改变
    bool changed = command->mode != motor_command.mode ||
                   command->a != motor_command.a ||
                   command->b != motor_command.b ||
                   command->action_id != motor_command.action_id;

    MotorState last_state = run_state;

    // 重复定时发送相同的指令不改变编号，不会重置PID或重启预设动作
    // 停止后重新启动，即使目标相同，也需要重新开始动作
    if (changed || (last_state != MOTOR_RUN && run_state == MOTOR_RUN))
        command_id++;

    // 保存最新一次命令
    motor_command = *command;
    upper_time = now;
    upper_connected = true;


    osKernelRestoreLock(lock_state);
}

// ---------------------------------根据指令控制电机------------------------------

void MotorTask(void *argument)
{
    (void)argument;

    uint32_t last_id = 0;

    while (1)
    {
        Command command = motor_command;
        uint32_t current_id = command_id;
        uint32_t feedback_time = motor_data.time_ms;
        uint32_t receive_time = upper_time;
        uint32_t now = HAL_GetTick();

        // 下位机与电调连接是否正常
        bool motor_ok = motor_data.received && (uint32_t)(now - feedback_time) < 100;
        // 上位机与下位机连接是否正常
        bool upper_ok = upper_connected && (uint32_t)(now - receive_time) < UPPER_TIMEOUT_MS;

        // 其中一个出问题就设置为不允许启动状态
        if (!motor_ok || !upper_ok)
            run_state = MOTOR_STOP;

        uint8_t state;

        if(command.mode==0x11||(command.mode == 0x22 && command.action_id == 0x11111111))
        {
            // 速度
            state=0xBA;
        }
        if(command.mode==0x22||(command.mode == 0x22 && command.action_id == 0x000000000))
        {
            // 位置
            state=0xAA;
        }

        // 重置
        if (run_state != MOTOR_RUN)
        {
            PID_Reset(&speedPID);
            PID_Reset(&positionPID);
            CAN_SendCurrent(0);
        }
        else
        {
            // 状态切换，重置
            if (current_id != last_id)
            {
                PID_Reset(&speedPID);
                PID_Reset(&positionPID);
                Action_Start(now);
                last_id = current_id;
            }

            // 位置模式与预设模式使用作业规定的电流上限
            speedPID.outputMax = MOTOR_CURRENT_LIMIT;

            // 使用状态机
            switch (command.mode)
            {
            case 0x00:
                // A为电流上限，B为输出轴目标转速
                // 限幅
                speedPID.outputMax = command.a;
                if (speedPID.outputMax > MOTOR_CURRENT_LIMIT)
                    speedPID.outputMax = MOTOR_CURRENT_LIMIT;

                speedServo((float)command.b);
                // 允许一定的误差
                if (fabsf(speedPID.ref - motor_data.output_speed) < 5.0f)
                    state = 0xAF;
                break;

            case 0x11:
                // A为最大输出轴转速，B为目标角度
                positionServo((float)command.b, (float)command.a);

                if (fabsf(positionPID.ref - motor_data.position) < 2.0f &&
                    fabsf(motor_data.output_speed) < 5.0f)
                    state = 0xBF;
                break;

            case 0x22:
            {
                float ref;
                bool finished;

                // 预设动作1
                if (command.action_id == 0x00000000)
                {
                    finished = SpeedAction(now, &ref);
                    speedServo(ref);
                    if (finished && fabsf(motor_data.output_speed) < 5.0f)
                        state = 0xAF;
                }
                // 预设动作2（可以用else是因为Decode里面已经丢弃其他错误指令了）
                else
                {
                    finished = PositionAction(now, motor_data.position,motor_data.output_speed, &ref);
                    positionServo(ref, 120.0f);
                    if (finished)
                        state = 0xBF;
                }
                break;
            }
            }
        }

        motor_state = motor_ok ? state : 0xFF;

        osDelay(2);
    }
}
