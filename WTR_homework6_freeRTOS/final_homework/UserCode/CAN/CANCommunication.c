#include "CANCommunication.h"
#include "can.h"

// 全局反馈数据，供其他任务读取
volatile MotorData motor_data = {0};

// 上一次转子的角度，单位deg
static float last_angle = 0.0f;

// 第一帧反馈的转子角度，做位置零点
static float start_angle = 0.0f;

// 转子跨越零点时累计的圈数，允许正反向跨越，可正可负
static int32_t rounds = 0;

/*-----------------------------------开启过滤器和CAN1-------------------------------------*/
// 启动过滤器和CAN1
bool CAN_Start(void)
{
    // 初始化
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;

    // 32位过滤器
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    // 不限制
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;

    // 进入FIFO0
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    HAL_CAN_ConfigFilter(&hcan1, &filter);

    // CAN1启动！
    HAL_CAN_Start(&hcan1);

    // 若成功开启通知，返回ture
    return HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK;
}

/*-----------------------------在收报回调中获取并且处理数据---------------------------------*/

// FIFO0收报回调函数
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 不是CAN1则退出
    if (hcan->Instance != CAN1)
    {
        return;
    }

    // 一个标准数据帧
    CAN_RxHeaderTypeDef header;

    // 数据正文
    uint8_t DATA[8];

    // 取出FIFO0中等待处理的报文（FIFO0内有数据才进入循环）
    while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0)
    {
        HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, DATA);

        // 只处理ID为1的电调发来的标准（IDE）数据（RTR）帧
        if (header.IDE != CAN_ID_STD || header.RTR != CAN_RTR_DATA ||
            // 接收反馈的报文ID，定义电调ID是一号，所以是0x201
            header.StdId != 0x201 ||
            // 数据长度是8
            header.DLC != 8)
        {
            // 不满足以上条件的跳过本帧
            continue;
        }

        // 大端序拼接转子角度
        uint16_t angle = ((uint16_t)DATA[0] << 8) | (uint16_t)DATA[1];

        // 大端序拼接转速编码（先拼起来，暂不考虑正负）
        uint16_t speed_temporary = ((uint16_t)DATA[2] << 8) | (uint16_t)DATA[3];

        // 将转速的16位编码还原成有符号数
        int16_t speed;
        if (speed_temporary >= 0x8000)
        {
            // 考虑负数
            speed = (int16_t)((int32_t)speed_temporary - 65536);
        }
        else
        {
            speed = (int16_t)speed_temporary;
        }

        // 再把数值换算成角度，作为最新的转子角度
        float rotor_angle = (float)angle * 360.0f / 8192.0f;

        // 角度值0-8191，对应0-360
        // 第一帧反馈作初始位置
        if (motor_data.received == false)
        {
            start_angle = rotor_angle;
            last_angle = rotor_angle;
            rounds = 0;
        }
        else
        {
            // 计算本次相对上次移动了多少计数
            float delta = rotor_angle - last_angle;

            // 正常反馈+空载转速下，相邻反馈之间达不到半圈
            // 反向跨过零点，圈数减1
            if (delta > 180.0f)
            {
                rounds--;
            }
            // 正向跨过零点，圈数加1
            else if (delta < -180.0f)
            {
                rounds++;
            }
        }

        // 保存本次角度，供下一次比较
        last_angle = rotor_angle;

        // 保存原始反馈
        motor_data.angle = angle;
        motor_data.speed = speed;

        // 把转子转速换算为输出轴转速（比例36：1），注意有正负
        motor_data.output_speed = MOTOR_DIRECTION * (float)speed / MOTOR_RATIO;

        // 当前转子相对于初始位置的绝对角度
        float total_angle;
        total_angle = rounds * 360.0f + rotor_angle - start_angle;

        // 换算输出轴相对初始位置的角度
        motor_data.position = MOTOR_DIRECTION * total_angle / MOTOR_RATIO;

        // 更新接收状态
        motor_data.time_ms = HAL_GetTick();
        motor_data.received = true;
    }
}


/*--------------------------------向电调发送电流指令-----------------------------------*/

// current 设置电调电流
// 向1号电调发送电流指令
bool CAN_SendCurrent(int16_t current)
{
    // 电流限幅（怀恋友好的第一次作业...）
    if (current > MOTOR_CURRENT_LIMIT)
    {
        current = MOTOR_CURRENT_LIMIT;
    }
    else if (current < -MOTOR_CURRENT_LIMIT)
    {
        current = -MOTOR_CURRENT_LIMIT;
    }

    // 与反馈数据使用同一个方向系数
    int16_t motor_current = MOTOR_DIRECTION * current;

    // 转成无符号编码，便于拆分字节
    uint16_t current_bits = (uint16_t)motor_current;

    // CAN发送报文头
    CAN_TxHeaderTypeDef header = {0};

    // 电调接收的时候id是0x200
    header.StdId = 0x200;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = 8;
    header.TransmitGlobalTime = DISABLE;

    uint8_t data[8] = {0};

    // 1号电调电流，大端序
    data[0] = (uint8_t)(current_bits >> 8);
    data[1] = (uint8_t)current_bits;

    // 发送邮箱
    uint32_t mailbox;

    // 将报文放入CAN发送邮箱
    return HAL_CAN_AddTxMessage(&hcan1, &header,data,&mailbox) == HAL_OK;
}