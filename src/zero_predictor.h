#pragma once

// A simple predictor that returns constant zero.

#include <array>

template <typename Elem>
class ZeroPredictor {
 public:
  using ElemType = Elem;
  Elem Encode(Elem val) { return 0; }
};
