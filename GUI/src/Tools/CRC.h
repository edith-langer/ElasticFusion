#pragma once

#include <stddef.h>
#include <stdint.h>

/** Compute CRC32 using Ethernet polynomial.
  * Implementation copied from: https://stackoverflow.com/a/27950866/1525865 */
uint32_t crc32(uint32_t crc, const char* buf, size_t len);
