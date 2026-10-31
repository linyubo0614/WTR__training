#include "action.h"
#include <math.h>

static uint32_t start_time = 0;
static uint32_t wait_time = 0;
static uint8_t step = 0;

void Action_Start(uint32_t now)
{
    start_time = now;
    wait_time = now;
    step = 0;
}

// 速度：60、120、180、240、300、150、0rpm 各一秒
bool SpeedAction(uint32_t now, float *ref)
{
    uint32_t pased_time = now - start_time;

    if (pased_time < 1000)
        *ref = 60.0f;
    else if (pased_time < 2000)
        *ref = 120.0f;
    else if (pased_time < 3000)
        *ref = 180.0f;
    else if (pased_time < 5000)
        *ref = 240.0f;
    else if (pased_time < 7000)
        *ref = 300.0f;
    else if (pased_time < 8000)
        *ref = 150.0f;
    else
        *ref = 0.0f;

    // 之后保持零转速
    return pased_time >= 9000;
}


static float start_position = 0.0f;

// 位置：先到0°，再到720°停3秒，360°停1秒，最后回0°
// 因为从一个位置到另一个位置的时间不固定
// 故需要判断是否到位了（判定范围需要调）
// 再开始等待指定时间
// 传入的是相对于初始零点的位置和当前速度
// 以当前位置为0点
// 利用指针改变PID的ref 
bool PositionAction(uint32_t now, float position, float speed, float *ref)
{
    switch (step)
    {
    case 0:
        // 记录最初的位置
        start_position = position;

        // 从起点正向转720
        *ref = start_position + 720.0f;
        step = 1;
        break;

    case 1:
        // 前往720°
        *ref = start_position + 720.0f;

        if (fabsf(*ref - position) < 2.0f && fabsf(speed) < 5.0f)
        {
            // 到位并停稳后开始计时
            wait_time = now;
            step = 2;
        }
        break;

    case 2:
        // 在起点+720°停留3秒
        *ref = start_position + 720.0f;

        if ((now - wait_time) >= 3000)
        {
            *ref = start_position + 360.0f;
            step = 3;
        }
        break;

    case 3:
        // 从起点+720°退回到起点+360°
        *ref = start_position + 360.0f;

        if (fabsf(*ref - position) < 8.0f && fabsf(speed) < 7.0f)
        {
            // 到位并停稳后开始计时
            wait_time = now;
            step = 4;
        }
        break;

    case 4:
        // 在起点+360°停留1秒
        *ref = start_position + 360.0f;

        if ((uint32_t)(now - wait_time) >= 1000)
        {
            *ref = start_position;
            step = 5;
        }
        break;

    case 5:
        // 回到本次动作的起点
        *ref = start_position;

        if (fabsf(*ref - position) < 5.0f && fabsf(speed) < 5.0f)
        {
            return true;
        }
        break;

    default:
        // 状态异常时暂时保持当前位置，下一轮重新开始
        *ref = position;
        step = 0;
        break;
    }

    return false;
}