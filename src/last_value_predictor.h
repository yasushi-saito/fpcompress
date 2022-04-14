#pragma once

// A simple predictor that just returns the last value seen.

#include <array>

template <typename Elem>
class LastValuePredictor {
 public:
  using ElemType = Elem;
// Called during compression. It provides the next value to compress.  It
  // returns value predicted for the next value. If the prediction is perfect,
  // the returned value is equal to dv itself.
  Elem Encode(Elem val) {
    const auto pred = last_;
    last_ = val;
    return pred;
  }

 private:
  Elem last_;
};
