#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 外部调用接口
 * 
 * data 开始计入crc的数据的第一位
 * length 计入crc的数据长度
 */
uint8_t Crc8(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif