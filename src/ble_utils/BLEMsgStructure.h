#ifndef BLEMSGSTRUCTURE_H
#define BLEMSGSTRUCTURE_H

#include <Arduino.h>

  

static uint16_t crc16(uint8_t *data, uint8_t size)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < size; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static bool checkCRC(uint8_t *data, uint8_t size)
{
    uint16_t crc = crc16(data, size-2);
    uint16_t crc2 = (data[size-2] << 8) | data[size-1];
    return crc == crc2;
}

#endif // BLEMSGSTRUCTURE_H