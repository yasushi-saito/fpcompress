#pragma once

// A simple predictor that just returns the last value seen.

#include <array>

template<typename Value>
class LastValuePredictor {
 public:
  // Called during compression. It provides the next value to compress.  It
  // returns value predicted for the next value. If the prediction is perfect,
  // the returned value is equal to dv itself.
  Value Encode(Value val) {
    const auto pred = last_;
    last_ = val;
    return pred;
  }

private:
  Value last_;
};
