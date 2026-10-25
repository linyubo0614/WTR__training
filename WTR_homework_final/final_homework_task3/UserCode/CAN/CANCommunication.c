#include "CANCommunication.h"
#include "can.h"

// 全局变量可供其他任务读取，记录最近一次反馈的数据
volatile MotorData motor_data = {0};

// 上一次转子的角度，单位deg
static float last_rotor_angle = 0.0f;

// 第一帧反馈的转子角度，做位置零点
static float start_angle = 0.0f;

// 转子跨越零点时累计的圈数，允许正反向跨越，可正可负
static int32_t rounds = 0;

// 零电流前取消尚未发出的旧非零指令
static bool current_pending = false;

// -----------------------------------开启过滤器和CAN1-------------------------------------
// 启动过滤器和CAN1
bool CAN_Start(void)
{
    // 初始化
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;

    // 32位过滤器
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    // 都不限制
    filter.FilterIdHigh = 0;
    filter.FilterIdLow = 0;
    filter.FilterMaskIdHigh = 0;
    filter.FilterMaskIdLow = 0;

    // 进入FIFO0
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;

    // 启动过滤器
    HAL_StatusTypeDef result = HAL_CAN_ConfigFilter(&hcan1, &filter);
    if (result != HAL_OK)
        return false;

    // CAN1启动！
    result = HAL_CAN_Start(&hcan1);
    if (result != HAL_OK)
        return false;

    // 开启通知，若成功，返回true
    return HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK;
}

// -----------------------------在收报回调中获取并且处理数据---------------------------------
/**
 * 用于获取电调反馈的数据并且处理
 * 存入motor_data结构体
 * 
 * last_rotor_angle   上次接收的反馈的角度
 * rotor_angle        最新接收的反馈的角度
 * angle              反馈的原始数，0-8191
 * total_angle        转子相对于初始位置的角度，可以大于360，可负
 * position           输出轴相对于初始位置的角度
 * speed              反馈的原始数据，rpm
*/
// FIFO0收报回调函数（FIFO0中收到报文）
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
        HAL_StatusTypeDef result = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, DATA);
        if (result != HAL_OK)
            break;

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

        // 不能超过手册限定数
        if (angle > 8191)
            continue;

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

        // 再换算成角度，作为最新的转子角度
        float rotor_angle = (float)angle * 360.0f / 8192.0f;

        // 角度值0-8191，对应0-360
        // 第一帧反馈作初始位置
        if (motor_data.received == false)
        {
            start_angle = rotor_angle;
            last_rotor_angle = rotor_angle;
            rounds = 0;
        }
        else
        {
            // 稍微算了下，正常反馈+空载转速下，相邻反馈之间达不到半圈
            //

            // 计算本次相对上次移动了多少度
            float delta = rotor_angle - last_rotor_angle;

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
        last_rotor_angle = rotor_angle;

        // 保存原始反馈
        motor_data.angle = angle;
        motor_data.speed = speed;

        // 把转子转速换算为输出轴转速（比例36：1），有正负（方向）
        motor_data.output_speed = MOTOR_DIRECTION * (float)speed / MOTOR_RATIO;

        // 当前转子相对于初始位置的绝对角度
        float total_angle=total_angle = rounds * 360.0f + rotor_angle - start_angle;

        // 换算输出轴相对初始位置的角度
        motor_data.position = MOTOR_DIRECTION * total_angle / MOTOR_RATIO;

        // 更新接收状态
        motor_data.time_ms = HAL_GetTick();
        motor_data.received = true;
    }
}


// --------------------------------向电调发送电流指令-----------------------------------
// 计算电流的任务交由串级PID
// 本函数主要供串级PID计算后调用，做积分限幅和数据帧包装，发送给CAN总线
// 设置为一号电调
bool CAN_SendCurrent(int16_t current)
{
    // 电流限幅（作业规定4000）
    if (current > MOTOR_CURRENT_LIMIT)
    {
        current = MOTOR_CURRENT_LIMIT;
    }
    else if (current < -MOTOR_CURRENT_LIMIT)
    {
        current = -MOTOR_CURRENT_LIMIT;
    }

    if (current == 0 && current_pending)
    {
        // 防止总线恢复时先发出故障前滞留的非零指令
        HAL_CAN_AbortTxRequest(&hcan1,
            CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2);
        current_pending = false;
    }

    // 与反馈数据使用同一个方向系数
    int16_t motor_current = MOTOR_DIRECTION * current;

    // 转成无符号数，便于拆分字节
    uint16_t current_bits = (uint16_t)motor_current;

    // CAN发送报文头
    CAN_TxHeaderTypeDef header = {0};

    // 1号电调接收的时候id是0x200
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
    HAL_StatusTypeDef result = HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox);
    if (result == HAL_OK && current != 0)
        current_pending = true;

    return result == HAL_OK;
}