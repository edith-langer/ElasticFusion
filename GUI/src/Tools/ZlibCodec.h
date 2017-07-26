#pragma once

#include <vector>
#include <stdint.h>

class ZLIBCodec
{
public:
  /** Decode a buffer using zlib.
    * Destination buffer should be large enough to hold the uncompressed data.
    * \param[in] src_data source buffer
    * \param[in] src_size source buffer size in bytes
    * \param[in] dst_data destination buffer, must be preallocated
    * \param[in] dst_size destination buffer size in bytes
    * \return size in bytes of uncompressed data. */
  static size_t
  decode(const uint8_t* src_data, size_t src_size, uint8_t* dst_data, size_t dst_size);

  static size_t
  decode(const std::vector<uint8_t>& src, uint8_t* dst_data, size_t dst_size);
};
