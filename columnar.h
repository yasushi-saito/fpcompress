#pragma once

#include "zstd.h"

#include "type.h"

#include <array>
#include <memory>
#include <cstring>

template<typename Elem, typename Predictor>
class ColumnarCompressor : public Compressor {
private:
  using Cast = Bitcast<Elem>;
  using Int = typename Cast::Int;
  const std::string name_;
  // Number of values in one logical unit. For example, tuple_size_=3 when the
  // data stores an array of 3D coordinates.
  const int tuple_size_;
  Predictor predictor_;
  std::unique_ptr<Compressor> zstd_;

public:
  ColumnarCompressor(const std::string& name, int tuple_size)
      : name_(name),tuple_size_(tuple_size), zstd_(NewZstd()) {}

  std::string Name() const { return name_; }
  Bytes Compress(const Bytes &src) {
    CHECK(src.size() % (sizeof(Elem) * tuple_size_) == 0);
    Bytes tmp;
    tmp.resize(src.size());

    size_t n_tuple = src.size() / (sizeof(Elem) * tuple_size_);
    //std::cerr << "SRC=" << src.size() << " tuplesize=" << tuple_size_ << "\n";
    //std::cerr << "NTUPLE=" << n_tuple << "\n";
    for (int i = 0; i < tuple_size_; i++) {
      for (size_t j = 0; j < n_tuple; j++) {
        size_t off = (j * tuple_size_ + i) * sizeof(Elem);
        Elem dorig = Cast::ToFloat(src.data() + off);
        Elem dpred = predictor_.Encode(dorig);

        Int iorig = Cast::ToInt(dorig);
        Int ipred = Cast::ToInt(dpred);

        Int enc = ipred ^ iorig;
#if 0
        if (j < 100) {
          const int ss0 = 48;
          const int ss1 = 64-ss0;
          std::cout << name_ << "" << j << ":" << std::hex << (enc >> ss0) <<
              "," << (enc <<ss1>>ss1) << std::dec << "\n";
        }
#endif
        std::array<uint8_t, sizeof(Elem)> nvbuf;
        memcpy(&nvbuf, &enc, sizeof(Elem));

        constexpr int kPrefix = 3;
        constexpr int kSuffix = sizeof(Elem)-kPrefix;
        auto exp_off = (j * tuple_size_+i) * kPrefix;
        for (int i = 0; i < kPrefix; i++) {
          tmp[exp_off + i] = nvbuf[kSuffix + i];
        }

        auto frac_off = n_tuple * tuple_size_ * kPrefix + (i+j*tuple_size_) * kSuffix;
        for (int i = 0; i < kSuffix; i++) {
          tmp[frac_off + i] = nvbuf[i];
        }
        // tmp.insert(tmp.end(), new_value_ptr, new_value_ptr + sizeof(Elem));
      }
    }
    CHECK(tmp.size() == src.size());
    return zstd_->Compress(tmp);
  }
  Bytes Decompress(const Bytes &src) { return src; }
};
