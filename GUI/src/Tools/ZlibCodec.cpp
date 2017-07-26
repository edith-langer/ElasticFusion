#include <zlib.h>

#include "ZlibCodec.h"

size_t
ZLIBCodec::decode(const uint8_t* src_data, size_t src_size, uint8_t* dst_data, size_t dst_size)
{
  unsigned long output_size = dst_size;
  auto error_code = uncompress(dst_data, &output_size, reinterpret_cast<const Bytef*>(src_data), src_size);
  if (error_code != Z_OK)
    throw "ZLIB codec failed to uncompress buffer";
  return output_size;
}

size_t
ZLIBCodec::decode(const std::vector<uint8_t>& src, uint8_t* dst_data, size_t dst_size)
{
  return decode(src.data(), src.size(), dst_data, dst_size);
}
