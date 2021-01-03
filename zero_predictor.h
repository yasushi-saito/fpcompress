#pragma once

// A simple predictor that returns constant zero.

#include <array>

template<typename Value>
class ZeroPredictor {
 public:
  Value Encode(Value val) {
    return 0;
  }
};
