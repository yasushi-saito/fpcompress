#pragma once

#include <array>
#include <cstring>


template <typename Elem>
class StridePredictor {
  Elem diff_;
  Elem last_;
 public:
  // Called during compression. It provides the next value to compress.  It
  // returns value predicted for the next value. If the prediction is perfect,
  // the returned value is equal to dv itself.
  Elem Encode(Elem dv) {
    const Elem pred = last_ + diff_;
    diff_ = dv - last_;
    last_ = dv;
    return pred;
  }
};
