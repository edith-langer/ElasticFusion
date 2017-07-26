#include "CRC.h"

uint32_t crc32(uint32_t crc, const char *buf, size_t len)
{
    static const uint32_t POLY = 0xedb88320;
    crc = ~crc;
    while (len--)
    {
        crc ^= *buf++;
        for (int k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
    }
    return ~crc;
}
