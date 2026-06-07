#pragma once

#include <stdint.h>
#include <stddef.h>

// DSMR 5.0.2 CRC16:
// - Polynomial: x^16 + x^15 + x^2 + 1  (0x8005 MSB-first)
// - Reflected polynomial for LSB-first engine: 0xA001
// - Init: 0x0000
// - No XOR in
// - No XOR out
// - No reflection of input or output
// - Bit order: LSB-first (shift right)

uint16_t dsmr_crc16(const uint8_t *data, size_t len);
