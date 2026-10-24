#include "crc.h"
#include "crc.hpp"

// 作业要求的参数
using CRC8_ATM = crc::CRCX<8, 0x07, 0x00, false, false, 0x00>;

// 可供外部调用计算crc函数
uint8_t Crc8(const uint8_t *data, uint16_t length)
{
    // 来自作业提供的crc.hpp
    return CRC8_ATM::calc(data, length);
}