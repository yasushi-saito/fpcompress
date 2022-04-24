#pragma once
#include <cstdint>
#include <cstdlib>

#include "bit_cast.h"

namespace fpcompress {

// The default encoder and decoder (for integers) return the value as is.
template <typename Elem>
class RotatingEncoder {
 public:
  using ElemType = Elem;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  DataType Encode(Elem v) { abort(); }
};

template <typename Elem>
class RotatingDecoder {
 public:
  using ElemType = Elem;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  DataType Encode(Elem v) { abort(); }
};

// The encoder for 32bit float moves the sign bit to the LSB side.  This makes
// the first byte to contain only the exp part.
//
// TODO(saito): try rotating just the first 9 bits.
//
// 32bit FP: 1bit sign, 8bit exp, 23bit fractional
// 64bit FP: 1bit sign, 11bit exp, 52bit fractional
template <>
class RotatingEncoder<float> {
 public:
  using ElemType = float;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  uint32_t old = 0;
  DataType Encode(ElemType v) {
    auto iv = Bitcast<ElemType>::ToMonotonicInt(v);
    uint32_t encoded = iv - old;
    old = iv;
    return ToBytes(encoded);
    // auto iv = Bitcast<ElemType>::ToInt(v);
    // return ToBytes(RotateLeft(iv, static_cast<uint32_t>(1)));
  }
};

template <>
class RotatingDecoder<float> {
 public:
  using ElemType = float;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  ElemType Decode(DataType v) {
    auto iv = FromBytes<uint32_t>(v);
    return Bitcast<ElemType>::ToFloat(
        RotateRight(iv, static_cast<uint32_t>(1)));
  }
};

template <>
class RotatingEncoder<double> {
 public:
  using ElemType = double;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  uint64_t old = 0;
  DataType Encode(ElemType v) {
    uint64_t iv = Bitcast<ElemType>::ToMonotonicInt(v);
    uint64_t encoded = iv - old;
    old = iv;
    return ToBytes(encoded);
    // return ToBytes(v);
  }
};

template <>
class RotatingDecoder<double> {
 public:
  using ElemType = double;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  ElemType Decode(DataType v) { return FromBytes<ElemType>(v); }
};

}  // namespace fpcompress
