#pragma once

#include <glog/logging.h>

#include <array>
#include <cstring>
#include <memory>
#include <span>

#include "absl/strings/str_format.h"
#include "bit_cast.h"
#include "combo_stream.h"
#include "output_stream.h"

namespace fpcompress {
template <typename Predictor>
class ColumnarOutputStream : public OutputStream {
 private:
  using Elem = typename Predictor::ElemType;
  using Cast = Bitcast<Elem>;
  using Int = typename Cast::Int;
  const std::string name_;

  static constexpr int kBufSize = 4 << 20;
  struct ElemOutput {
    Predictor predictor;
    std::array<std::unique_ptr<OutputStream>, sizeof(Elem)> out;
    std::array<std::vector<uint8_t>, sizeof(Elem)> buf;
  };
  std::unique_ptr<ComboOutputStream> combo_out_;

  std::vector<ElemOutput> tuple_outs_;

  /*
  std::vector<Predictor> predictor_;
  struct Output {
    std::unique_ptr<OutputStream> out;
    std::vector<uint8_t> buf;
  };
  std::vector<Output> outs_;
  */
 public:
  ColumnarOutputStream(std::string name, int tuple_size,
                       ComboOutputStream::Factory factory,
                       std::unique_ptr<OutputStream> out)
      : name_(std::move(name)) {
    std::vector<ComboOutputStream::OutputSpec> spec(tuple_size * sizeof(Elem));
    for (int i = 0; i < spec.size(); i++) {
      spec[i].name = absl::StrFormat("b%d", i);
      spec[i].factory = factory;
    }

    combo_out_ = std::make_unique<ComboOutputStream>(std::move(out));
    auto column_outs = combo_out_->Init(spec);

    CHECK_EQ(spec.size(), column_outs.size());
    tuple_outs_.resize(tuple_size);
    for (int ti = 0; ti < tuple_size; ti++) {
      for (int j = 0; j < sizeof(Elem); j++) {
        tuple_outs_[ti].out[j] = std::move(column_outs[ti * sizeof(Elem) + j]);
        tuple_outs_[ti].buf[j].reserve(kBufSize);
      }
    }
    /*
    predictor_.resize(tuple_size);
    outs_.resize(outs.size());
    for (int i = 0; i < outs_.size(); i++) {
      outs_[i].out = std::move(outs[i]);
      outs_[i].buf.reserve(kBufSize);
      }*/
  }

  ~ColumnarOutputStream() {
    Flush();
    combo_out_.reset();
  }

  std::string Name() const { return name_; }

  void Flush() override {
    for (auto& to : tuple_outs_) {
      for (int i = 0; i < sizeof(Elem); i++) {
        to.out[i]->Append(to.buf[i]);
        to.buf[i].clear();
      }
    }
    combo_out_->NextBlock();
  }

  void Append(std::span<const uint8_t> src) override {
    CHECK(src.size() % (tuple_outs_.size() * sizeof(Elem)) == 0);
    // std::vector<uint8_t> tmp;
    // tmp.resize(src.size());

    const size_t n_tuples = src.size() / (tuple_outs_.size() * sizeof(Elem));

    for (size_t j = 0; j < n_tuples; j++) {
      for (size_t ti = 0; ti < tuple_outs_.size(); ti++) {
        auto& to = tuple_outs_[ti];
        size_t off = (ti + j * tuple_outs_.size()) * sizeof(Elem);
        Elem dorig = Cast::ToFloat(src.data() + off);
        Elem dpred = to.predictor.Encode(dorig);

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
        const auto nvbuf = ToBytes(enc);

        for (int i = 0; i < sizeof(Elem); i++) {
          to.buf[i].push_back(nvbuf[i]);
        }

        if (to.buf[0].size() >= kBufSize) {
          for (int i = 0; i < sizeof(Elem); i++) {
            to.out[i]->Append(to.buf[i]);
            to.buf[i].clear();
          }
        }
      }
    }
  }
};
}  // namespace fpcompress
