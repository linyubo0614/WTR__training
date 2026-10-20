#include "process_task.h"
#include "cmsis_os2.h"
#include "data_types.h"
#include "queue_handles.h"
#include "fusion.h"

#include <string.h>

/* 处理任务的四种状态 */
typedef enum
{
    PROCESS_WAIT_BOTH,
    PROCESS_WAIT_SENSOR1,
    PROCESS_WAIT_SENSOR2,
    PROCESS_COMPARE
} ProcessState;

/* 定义初始状态 */
volatile ProcessState process_state = PROCESS_WAIT_BOTH;
typedef enum
{
    OUTPUT_RAW = 0,  /* 分别输出两路原始数据 */
    OUTPUT_FUSED =1   /* 输出融合数据 */
} OutputMode;

/* 默认状态设置为原始态 */
volatile OutputMode output_mode = OUTPUT_RAW;

/* flag判断是否收到切换状态的请求（也就是按键） */
#define PROCESS_SWITCH_FLAG  1

extern osThreadId_t processTaskHandle;

/* 申请切换模式函数，供按键任务调用 */
void Process_RequestModeSwitch(void)
{
    //将flag置1
    osThreadFlagsSet(processTaskHandle, PROCESS_SWITCH_FLAG);
}

/* 从指定队列读取一条数据,成功返回1.没有数据或读取失败返回 0*/

/* 从队列queue中获取信息，转化成结构体存到指定位置sample */
static uint8_t ReadSample(osMessageQueueId_t queue,SensorData *sample)
{

    SensorPacket packet;

    osStatus_t result = osMessageQueueGet(
        queue, packet, NULL, 0);

    /* 没有数据是正常情况 */
    if (result == osErrorResource)
    {
        return 0;
    }

    if (result != osOK)
    {
        return 0;
    }

    /* 合成结构体 */
    sample->type = packet[0];

    memcpy(&sample->value,
           &packet[1],
           sizeof(sample->value));

    memcpy(&sample->time_ms,
           &packet[5],
           sizeof(sample->time_ms));

    return 1;
}

/* 将融合后的数据送到串口队列 */
/* 这里传入的是融合后的数据 */
static void SendSample(const SensorData *sample)
{
    SensorPacket packet;

    packet[0] = sample->type;

    memcpy(&packet[1],
           &sample->value,
           sizeof(sample->value));

    memcpy(&packet[5],
           &sample->time_ms,
           sizeof(sample->time_ms));

    osMessageQueuePut(
        TxDateHandle, packet, 0, 0);
}



/* 用状态机实现数据融合 */
void ProcessTask(void *argument)
{
    (void)argument;

    /* 保留两路待配对的数据 */
    SensorData sample1;
    SensorData sample2;

    /* 存放融合结果 */
    SensorData fused;

    process_state = PROCESS_WAIT_BOTH;

    for (;;)
    {
        /*检查是否收到切换请求*/
        uint32_t flags = osThreadFlagsWait(
            PROCESS_SWITCH_FLAG,
            osFlagsWaitAny,
            0
        );

        /* 成功收到切换标志 */

        /* 当flags没有错误而且为1的时候 */
        if (((flags & osFlagsError) == 0) && ((flags & PROCESS_SWITCH_FLAG) != 0))
        {
            /* 状态切换 */
            if (output_mode == OUTPUT_FUSED)
            {
                output_mode = OUTPUT_RAW;
            }
            else
            {
                output_mode = OUTPUT_FUSED;
            }

            /* 放弃原有进度 */
            process_state = PROCESS_WAIT_BOTH;
        }

        /*原始模式：两路数据分别转发*/
        if (output_mode == OUTPUT_RAW)
        {
            if (ReadSample(Sensor01Handle, &sample1))
            {
                SendSample(&sample1);
            }

            if (ReadSample(Sensor02Handle, &sample2))
            {
                SendSample(&sample2);
            }

            osDelay(1);

            /* 跳出，不再参与后续的状态机 */
            continue;
        }

        /*融合模式，继续执行下面原有的switch*/
        switch (process_state)
        {
        case PROCESS_WAIT_BOTH:
            /* 先尝试第一个传感器 */
            if (ReadSample(Sensor01Handle, &sample1))
            {
                process_state = PROCESS_WAIT_SENSOR2;
            }
            /* 第一个没有数据，再尝试第二个 */
            else if (ReadSample(Sensor02Handle, &sample2))
            {
                process_state = PROCESS_WAIT_SENSOR1;
            }
            break;

        case PROCESS_WAIT_SENSOR1:
            /* sample2保持不变，只补充第一个 */
            if (ReadSample(Sensor01Handle, &sample1))
            {
                process_state = PROCESS_COMPARE;
            }
            break;

        case PROCESS_WAIT_SENSOR2:
            /* sample1保持不变，只补充第二个 */
            if (ReadSample(Sensor02Handle, &sample2))
            {
                process_state = PROCESS_COMPARE;
            }
            break;

        case PROCESS_COMPARE:
        {
            FusionResult result = Fusion_Calculate(&sample1, &sample2, &fused);

            if (result == FUSION_OK)
            {
                SendSample(&fused);

                /* 两条数据都已使用，开始下一组 */
                process_state = PROCESS_WAIT_BOTH;
            }
            else if (result == FUSION_DROP_SENSOR1)
            {
                /* 放弃旧sample1，保留sample2 */
                process_state = PROCESS_WAIT_SENSOR1;
            }
            else if (result == FUSION_DROP_SENSOR2)
            {
                /* 放弃旧sample2，保留sample1 */
                process_state = PROCESS_WAIT_SENSOR2;
            }

            break;
        }

        default:
            process_state = PROCESS_WAIT_BOTH;
            break;
        }

        osDelay(1);
    }
}