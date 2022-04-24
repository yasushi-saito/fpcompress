#pragma once
#include <cstdint>

#include "bit_cast.h"

namespace fpcompress {

// The default encoder and decoder (for integers) return the value as is.
template <typename Elem>
class IdEncoder {
 public:
  using ElemType = Elem;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  DataType Encode(Elem v) { return ToBytes(v); }
};

template <typename Elem>
class IdDecoder {
 public:
  using ElemType = Elem;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  DataType Encode(Elem v) { return FromBytes(v); }
};

}  // namespace fpcompress
