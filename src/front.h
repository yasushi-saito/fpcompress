#pragma once
/*
  From fpzip.

BSD 3-Clause License

Copyright (c) 2018-2019, Lawrence Livermore National Security, LLC
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdint>
#include <vector>

namespace fpcompress {
// front of encoded but not finalized samples
template <typename T>
class Front {
 public:
  Front(size_t nx, size_t ny, T zero = 0)
      : nx_(nx),
        ny_(ny),
        zero(zero),
        dx(1),
        dy(nx + 1),
        dz(dy * (ny + 1)),
        m(Mask(dx + dy + dz)),
        i(0),
        a_(m + 1) {}
  Front() = default;
  ~Front() = default;
  Front(const Front& src) = default;
  Front& operator=(const Front& src) = default;

  size_t XWidth() const { return nx_; }
  size_t YWidth() const { return ny_; }

  // fetch neighbor relative to current sample
  T Get(size_t x, size_t y, size_t z) const {
    return a_[(i - dx * x - dy * y - dz * z) & m];
  }

  // add n copies of sample f to front
  void Push(T f, size_t n = 1) {
    do {
      a_[i++ & m] = f;
    } while (--n);
  }

  // advance front to (x, y, z) relative to current sample and fill with zeros
  void Advance(size_t x, size_t y, size_t z) {
    Push(zero, dx * x + dy * y + dz * z);
  }

 private:
  size_t nx_;
  size_t ny_;
  T zero;             // default value
  size_t dx;          // front index x offset
  size_t dy;          // front index y offset
  size_t dz;          // front index z offset
  size_t m;           // index mask
  size_t i;           // modular index of current sample
  std::vector<T> a_;  // circular array of samples

  // return m = 2^k - 1 >= n - 1
  static size_t Mask(size_t n) {
    for (n--; n & (n + 1); n |= n + 1)
      ;
    return n;
  }
};
}  // namespace fpcompress
