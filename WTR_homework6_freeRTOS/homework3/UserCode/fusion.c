#include "fusion.h"

FusionResult Fusion_Calculate(
    const SensorData *sensor1,
    const SensorData *sensor2,
    SensorData *output
)
{
    /* 正数表示第一个较新,负数表示第二个较新 */
    int32_t difference =(int32_t)(sensor1->time_ms - sensor2->time_ms);

    /* 第一个比第二个晚超过500ms，第二个数据太早 */
    if (difference > 500) 
    {
        return FUSION_DROP_SENSOR2;
    }

    /* 第二个比第一个晚超过500ms，第一个数据太早 */
    if (difference < -500)
    {
        return FUSION_DROP_SENSOR1;
    }

    /* 时间差在允许范围内，计算平均值 */
    /* 融合后设置为第三种类型的数据 */
    output->type = 3;
    output->value =(sensor1->value + sensor2->value) / 2.0f;

    /* 使用较新的时间戳 */
    if (difference >= 0)
    {
        output->time_ms = sensor1->time_ms;
    }
    else
    {
        output->time_ms = sensor2->time_ms;
    }

    return FUSION_OK;
}