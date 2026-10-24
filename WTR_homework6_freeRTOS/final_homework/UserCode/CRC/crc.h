#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 调用接口
uint8_t Crc8(const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif