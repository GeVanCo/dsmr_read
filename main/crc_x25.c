#include "crc_x25.h"

uint16_t crc_x25(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8408;   // reversed 0x1021
            else
                crc >>= 1;
        }
    }

    return ~crc;  // final XOR
}
