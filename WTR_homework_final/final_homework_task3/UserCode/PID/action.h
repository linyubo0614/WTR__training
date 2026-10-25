#ifndef ACTION_H
#define ACTION_H

#include <stdbool.h>
#include <stdint.h>

// 开始一组新的预设动作
void Action_Start(uint32_t now);

// 返回本次目标值；返回true表示动作完成
bool SpeedAction(uint32_t now, float *ref);
bool PositionAction(uint32_t now, float position, float speed, float *ref);

#endif
