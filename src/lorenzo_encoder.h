#pragma once
#include <glog/logging.h>

#include <cstdint>

#include "bit_cast.h"
#include "front.h"

namespace fpcompress {

template <typename ElemType>
class LorenzoEncoder1D {
 public:
  using Elem = ElemType;
  using Int = Bitcast<Elem>::Int;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  DataType Encode(ElemType v) {
    Int iv = Bitcast<ElemType>::ToMonotonicInt(v);
    Int encoded = iv ^ old_;
    old_ = iv;
    return ToBytes(encoded);
  }

 private:
  Int old_ = 0;
};

template <typename ElemType>
class LorenzoEncoder2D {
 public:
  using Elem = ElemType;
  using Int = Bitcast<Elem>::Int;
  using DataType = std::array<uint8_t, sizeof(Elem)>;
  explicit LorenzoEncoder2D(int x_width) : x_width_(x_width) {
    buf_.resize(x_width * 2);
  }
  LorenzoEncoder2D() = default;
  LorenzoEncoder2D(const LorenzoEncoder2D<ElemType>& src) = default;
  LorenzoEncoder2D<ElemType>& operator=(const LorenzoEncoder2D<ElemType>& src) =
      default;
  DataType Encode(Elem fv) {
    Int pred = Bitcast<Elem>::ToMonotonicInt(Get(1, 0) + Get(0, 1) - Get(1, 1));
    Int val = Bitcast<Elem>::ToMonotonicInt(fv);
    buf_[x_++] = fv;
    if (x_ >= buf_.size()) x_ = 0;
    return ToBytes(val - pred);
  }

 private:
  Elem Get(int x_off, int y_off) const {
    auto off = x_ - x_off - y_off * x_width_;
    if (off < 0) off += buf_.size();
    CHECK_GE(off, 0);
    return buf_[off];
  }
  int x_width_ = 0;
  int64_t x_ = 0;
  std::vector<Elem> buf_;
};

// The default encoder and decoder (for integers) return the value as is.
template <typename ElemType>
class LorenzoEncoder3D {
 public:
  using Elem = ElemType;
  using Int = Bitcast<Elem>::Int;
  using DataType = std::array<uint8_t, sizeof(ElemType)>;
  LorenzoEncoder3D() = default;
  LorenzoEncoder3D(int x_width, int y_width)
      : x_width_(x_width), y_width_(y_width), front_(x_width, y_width) {
    front_.Advance(0, 0, 1);
    front_.Advance(0, 1, 0);
    front_.Advance(1, 0, 0);
  }
  LorenzoEncoder3D(const LorenzoEncoder3D& src) = default;
  LorenzoEncoder3D& operator=(const LorenzoEncoder3D& src) = default;

  DataType Encode(Elem fv) {
    Int pred = Bitcast<Elem>::ToMonotonicInt(
        front_.Get(1, 1, 1) + front_.Get(1, 0, 0) - front_.Get(0, 1, 1) +
        front_.Get(0, 1, 0) - front_.Get(1, 0, 1) + front_.Get(0, 0, 1) -
        front_.Get(1, 1, 0));
    Int val = Bitcast<Elem>::ToMonotonicInt(fv);
    front_.Push(fv);
    off_++;
    if (off_ % x_width_ == 0) {
      if (off_ % (x_width_ * y_width_) == 0) {
        front_.Advance(0, 1, 0);
      }
      front_.Advance(1, 0, 0);
    }
    return ToBytes(val - pred);
  }

 private:
  int x_width_ = 0;
  int y_width_ = 0;
  Front<Elem> front_;
  size_t off_ = 0;
  std::vector<Int> buf_;
};

}  // namespace fpcompress
