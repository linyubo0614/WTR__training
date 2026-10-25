#include "action.h"
#include <math.h>

static uint32_t start_time = 0;
static uint32_t wait_time = 0;
static uint8_t step = 0;
static bool waiting = false;

void Action_Start(uint32_t now)
{
    start_time = now;
    wait_time = now;
    step = 0;
    waiting = false;
}

// 速度：60、120、180、240、300、150、0rpm 各一秒
bool SpeedAction(uint32_t now, float *ref)
{
    uint32_t elapsed = now - start_time;

    if (elapsed < 1000)
        *ref = 60.0f;
    else if (elapsed < 2000)
        *ref = 120.0f;
    else if (elapsed < 3000)
        *ref = 180.0f;
    else if (elapsed < 5000)
        *ref = 240.0f;
    else if (elapsed < 7000)
        *ref = 300.0f;
    else if (elapsed < 8000)
        *ref = 150.0f;
    else
        *ref = 0.0f;

    // 第8至9秒执行零转速阶段，之后保持零转速
    return elapsed >= 9000;
}

// 位置序列：先到0°，再到720°停3秒、360°停1秒，最后回0°
bool PositionAction(uint32_t now, float position, float speed, float *ref)
{
    static const float target[4] = {0.0f, 720.0f, 360.0f, 0.0f};
    static const uint32_t duration[4] = {0, 3000, 1000, 0};

    *ref = target[step];

    bool reached = fabsf(*ref - position) < 2.0f && fabsf(speed) < 5.0f;

    if (!reached)
    {
        // 停留期间离开允许范围，则重新计算连续停留时间
        waiting = false;
        return false;
    }

    if (!waiting)
    {
        wait_time = now;
        waiting = true;
    }

    if ((uint32_t)(now - wait_time) >= duration[step])
    {
        if (step == 3)
            return true;

        step++;
        waiting = false;
        *ref = target[step];
    }

    return false;
}
