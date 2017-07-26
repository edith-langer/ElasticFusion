#pragma once

#include <vector>
#include <stdint.h>


class JPEGCodec
{
public:

  static size_t
  decode(const std::vector<uint8_t>& src, uint8_t* dst_data);
};
