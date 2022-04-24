#pragma once
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <type_traits>
#include <vector>

// Helper for bit-casting between a floating point number and a uint of the same
// size.
template <typename Value>
class Bitcast;

// https://stackoverflow.com/questions/776508/best-practices-for-circular-shift-rotate-operations-in-c
template <typename T>
T RotateLeft(T n, T shift) {
  const T mask = 8 * sizeof(T) - 1;

  shift &= mask;
  return (n << shift) | (n >> ((-shift) & mask));
}

template <typename T>
T RotateRight(T n, T shift) {
  const T mask = 8 * sizeof(T) - 1;
  shift &= mask;
  return (n >> shift) | (n << ((-shift) & mask));
}

// Cast the given value to a byte array.
template <typename T>
std::array<uint8_t, sizeof(T)> ToBytes(T v) {
  static_assert(std::is_trivial<T>::value);
  std::array<uint8_t, sizeof(T)> buf;
  memcpy(&buf[0], &v, sizeof(v));
  return buf;
}

// Cast the given byte array to a value.
template <typename T>
T FromBytes(std::span<const uint8_t> data) {
  static_assert(std::is_trivial<T>::value);
  T v;
  memcpy(&v, data.data(), data.size());
  return v;
}

template <>
class Bitcast<double> {
 public:
  using Int = uint64_t;
  using Float = double;
  static_assert(sizeof(Int) == sizeof(Float));

  static Int ToInt(const uint8_t *buf) {
    Int iv;
    memcpy(&iv, buf, sizeof(Int));
    return iv;
  }

  static Int ToInt(Float dv) {
    Int iv;
    memcpy(&iv, &dv, sizeof(Int));
    return iv;
  }

  static Float ToFloat(Int iv) {
    Float dv;
    memcpy(&dv, &iv, sizeof(Int));
    return dv;
  }
  static Float ToFloat(const uint8_t *buf) {
    Float dv;
    memcpy(&dv, buf, sizeof(Int));
    return dv;
  }
  static Int ToMonotonicInt(Float v) {
    Int iv = ToInt(v);
    if (iv & 0x8000000000000000UL) {
      iv = ~iv;
    } else {
      iv |= 0x8000000000000000UL;
    }
    return iv;
  }
};

template <>
class Bitcast<float> {
 public:
  using Int = uint32_t;
  using Float = float;

  static_assert(sizeof(Int) == sizeof(Float));

  static Int ToInt(const uint8_t *buf) {
    Int iv;
    memcpy(&iv, buf, sizeof(Int));
    return iv;
  }

  static Int ToInt(Float dv) {
    Int iv;
    memcpy(&iv, &dv, sizeof(Int));
    return iv;
  }

  static Float ToFloat(Int iv) {
    Float dv;
    memcpy(&dv, &iv, sizeof(Int));
    return dv;
  }
  static Float ToFloat(const uint8_t *buf) {
    Float dv;
    memcpy(&dv, buf, sizeof(Int));
    return dv;
  }

  static Int ToMonotonicInt(Float v) {
    Int iv = ToInt(v);
    if (iv & 0x80000000U) {
      iv = ~iv;
    } else {
      iv |= 0x80000000U;
    }
    return iv;
  }
};

// Fowler-Noll-Vo hash.
//
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
inline size_t Fnv1aHash(const void *src, size_t n_bytes) {
  static_assert(sizeof(size_t) == 8);
  size_t hash = 14695981039346656037u;
  const unsigned char *data = static_cast<const unsigned char *>(src);
  for (size_t i = 0; i < n_bytes; ++i) {
    hash = (hash ^ data[i]) * 1099511628211u;
  }
  return hash;
}
