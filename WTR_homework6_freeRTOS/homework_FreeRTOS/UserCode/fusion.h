#ifndef FUSION_H
#define FUSION_H

#include "data_types.h"

/* 枚举表示融合判断的三种结果 */
typedef enum
{
    FUSION_OK,           /* 融合成功 */
    FUSION_DROP_SENSOR1, /* 第一个数据太早，需要丢弃 */
    FUSION_DROP_SENSOR2  /* 第二个数据太早，需要丢弃 */
} FusionResult;

/* 判断并计算融合数据 */
FusionResult Fusion_Calculate(
    const SensorData *sensor1,
    const SensorData *sensor2,
    SensorData *output
);

#endif