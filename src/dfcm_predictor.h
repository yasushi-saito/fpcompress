#pragma once

// Fast lossless compression of scientific floating-point data
//
// Paruj Ratanaworabhan, Jian Ke, and Martin Burtscher
//
// DCC 2006
//
// https://ieeexplore.ieee.org/document/1607248
//
#include <array>
#include <cstring>

#include "bit_cast.h"
#include "parse.h"

template <typename Elem>
class DfcmPredictor {
  using Cast = Bitcast<Elem>;
  using Int = typename Cast::Int;

  // Direct-addressed cache keyed by the last three strides.
  static constexpr int kCacheSize = 65536;
  struct CacheEntry {
    size_t hash;
    Elem diff0; // The last diff
    Elem diff1; // The penultimate diff
  };
  std::array<CacheEntry, kCacheSize> cache_ = {{0}};

  // The delta between the last four values.
  // diff_[0] : last seen value - 2nd-to-last seen value
  // diff_[1] : 2nd-to-last seen value - 3rd-to-last seen value
  // diff_[1] : 3nd-to-last seen value - 4th-to-last seen value
  std::array<uint16_t, 3> diff_ = {0, 0, 0};

  // Last seen value.
  Elem last_ = 0.0;

  static size_t Hash(const std::array<uint16_t, 3>& diff) {
    return  Fnv1aHash(diff.data(), diff.size()*sizeof(uint16_t));
  }

 public:
  using ElemType = Elem;
  // Called during compression. It provides the next value to compress.  It
  // returns value predicted for the next value. If the prediction is perfect,
  // the returned value is equal to dv itself.
  Elem Encode(Elem dv) {
    const size_t hash = Hash(diff_);
    const size_t idx = hash % kCacheSize;
     Elem pred = last_;
     auto *const ent = &cache_[idx];
     if (ent->hash == hash) {
       auto fp0 = Cast::ToInt(ent->diff0) >> 50;
       auto fp1 = Cast::ToInt(ent->diff1) >> 50;
       if (fp0!=fp1) {
         pred = last_;
       } else {
         pred += ent->diff0;
       }
     }
     ent->diff1 = ent->diff0;
     ent->diff0 = dv - last_;
     ent->hash = hash;

     diff_[2] = diff_[1];
     diff_[1] = diff_[0];
     diff_[0] = (Cast::ToInt(dv) >> 50) - (Cast::ToInt(last_) >> 50);
     last_ = dv;
     return pred;
  }
};
