#include "dsmr_crc16.h"

uint16_t dsmr_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0x0000;
    const uint16_t poly = 0xA001; // reflected 0x8005

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
        }
    }

    return crc; // DSMR: no xorout, no reflection
}
