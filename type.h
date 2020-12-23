#pragma once
#include <iostream>
#include <cstring>
#include <memory>
#include <vector>

using Bytes = std::vector<uint8_t>;

// Interface for generic compressor.
class Compressor {
 public:
  // Returns a human-readable name of this compressor. For logging only.
  virtual std::string Name() const = 0;
  // Compress the given sequence of bytes.
  virtual Bytes Compress(const Bytes& data) = 0;
  // Inverse of Compress.
  virtual Bytes Decompress(const Bytes& data) = 0;
};

extern std::unique_ptr<Compressor> NewZstd();

// Helper for bit-casting between a floating point number and a uint of the same
// size.
template<typename Value> class Bitcast;

template <> class Bitcast<double> {
 public:
  using Int = uint64_t;
  using Float = double;
  static_assert(sizeof(Int)==sizeof(Float));

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
};

template <> class Bitcast<float> {
  using Int = uint32_t;
  using Float = float;

  static_assert(sizeof(Int)==sizeof(Float));

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
};

// Fowler-Noll-Vo hash.
//
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
inline size_t Fnv1aHash(const void* src, size_t n_bytes) {
  static_assert(sizeof(size_t) == 8);
  size_t hash = 14695981039346656037u;
  const unsigned char* data = static_cast<const unsigned char*>(src);
  for (size_t i = 0; i < n_bytes; ++i) {
    hash = (hash ^ data[i]) * 1099511628211u;
  }
  return hash;
}


#define CHECK(cond)                                                            \
  if (!(cond)) {                                                               \
    std::cerr << #cond << "\n";                                                \
    abort();                                                                   \
  }
