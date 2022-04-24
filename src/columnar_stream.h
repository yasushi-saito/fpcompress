#pragma once

#include <glog/logging.h>

#include <array>
#include <cstring>
#include <memory>
#include <span>

#include "absl/strings/str_format.h"
#include "bit_cast.h"
#include "combo_stream.h"
#include "const.h"
#include "input_stream.h"
#include "output_stream.h"

namespace fpcompress {
template <typename Encoder>
class ColumnarOutputStream : public OutputStream {
 private:
  using Elem = typename Encoder::Elem;
  // using Cast = Bitcast<Elem>;
  // using Int = typename Cast::Int;
  const std::string name_;
  // Encoder for one dimension. If the input is just a sequence of floats, then
  // elem_outs_.size()=1. If the input is a sequence of three floats for
  // (x,y,z), then elem_outs_.size()==3.
  struct ElemOutput {
    Encoder encoder;
    std::array<std::unique_ptr<OutputStream>, sizeof(Elem)> out;
    std::array<std::vector<uint8_t>, sizeof(Elem)> buf;
  };
  std::unique_ptr<ComboOutputStream> combo_out_;
  std::vector<ElemOutput> elem_outs_;

 public:
  ColumnarOutputStream(std::string name, const Encoder& encoder, int tuple_size,
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
    elem_outs_.resize(tuple_size);
    for (int ti = 0; ti < tuple_size; ti++) {
      elem_outs_[ti].encoder = encoder;
      for (int j = 0; j < sizeof(Elem); j++) {
        elem_outs_[ti].out[j] = std::move(column_outs[ti * sizeof(Elem) + j]);
        elem_outs_[ti].buf[j].reserve(kBufSize);
      }
    }
  }

  ~ColumnarOutputStream() {
    Flush();
    combo_out_.reset();
  }

  std::string Name() const { return name_; }

  void Flush() override {
    for (auto& to : elem_outs_) {
      for (int i = 0; i < sizeof(Elem); i++) {
        to.out[i]->Append(to.buf[i]);
        to.buf[i].clear();
      }
    }
    combo_out_->NextBlock();
  }

  void Append(std::span<const uint8_t> src) override {
    CHECK(src.size() % (elem_outs_.size() * sizeof(Elem)) == 0);
    // std::vector<uint8_t> tmp;
    // tmp.resize(src.size());

    const size_t n_tuples = src.size() / (elem_outs_.size() * sizeof(Elem));

    for (size_t j = 0; j < n_tuples; j++) {
      for (size_t ti = 0; ti < elem_outs_.size(); ti++) {
        auto& to = elem_outs_[ti];
        size_t off = (ti + j * elem_outs_.size()) * sizeof(Elem);
        const auto dorig = FromBytes<Elem>(
            std::span<const uint8_t>(src.data() + off, sizeof(Elem)));
        const auto encoded = to.encoder.Encode(dorig);

        for (int i = 0; i < encoded.size(); i++) {
          const uint8_t byte = encoded[i];
          to.buf[i].push_back(byte);
        }

        if (to.buf[0].size() >= kBufSize) {
          for (int i = 0; i < sizeof(Elem); i++) {
            to.out[i]->Append(to.buf[i]);
            to.buf[i].clear();
          }
          Flush();
        }
      }
    }
  }
};

template <typename Decoder>
class ColumnarInputStream : public InputStream {
 private:
  using Elem = typename Decoder::ElemType;
  // using Cast = Bitcast<Elem>;
  // using Int = typename Cast::Int;

 public:
  ColumnarInputStream(std::string name, int tuple_size,
                      ComboInputStream::Factory factory,
                      std::unique_ptr<InputStream> in)
      : name_(name),
        combo_in_(std::make_unique<ComboInputStream>(std::move(in))) {
    auto inputs = combo_in_->Init(factory);
    tuple_inputs_.resize(tuple_size);
    for (int ti = 0; ti < tuple_size; ti++) {
      ElemInput* in = &tuple_inputs_[ti];
      for (int j = 0; j < sizeof(Elem); j++) {
        in->in[j] = std::move(inputs[ti * sizeof(Elem) + j]);
        in->old[j] = 0;
      }
    }
  }

  std::string Name() const { return name_; }
  std::span<const uint8_t> Read(int bytes) {
    while (next_pos_ + bytes > buf_.size()) {
      if (!buf_.empty()) {
        const auto new_size = buf_.size() - next_pos_;
        memmove(&buf_[0], &buf_[next_pos_], new_size);
        buf_.resize(new_size);
      }
      auto old_size = buf_.size();
      NextBlock(bytes);
      CHECK_GE(buf_.size(), old_size);
      if (buf_.size() == old_size) return std::span<const uint8_t>();
    }
    size_t n = std::min<size_t>(bytes, buf_.size() - next_pos_);
    std::span<const uint8_t> r(&buf_[0] + next_pos_, n);
    next_pos_ += n;
    return r;
  }

 private:
  struct ElemInput {
    Decoder decoder;
    std::array<uint8_t, sizeof(Elem)> old;
    std::array<std::unique_ptr<InputStream>, sizeof(Elem)> in;
    std::array<std::span<const uint8_t>, sizeof(Elem)> buf;
  };

  void NextBlock(int bytes) {
    if (!combo_in_->NextBlock()) return;
    ssize_t n_elems = -1;
    for (auto& in : tuple_inputs_) {
      for (int j = 0; j < sizeof(Elem); j++) {
        // TODO(saito): kBufSize must be written in the file header.
        in.buf[j] = in.in[j]->Read(std::max(bytes, kBufSize));
        CHECK(n_elems < 0 || n_elems == in.buf[j].size());
        n_elems = in.buf[j].size();
      }
    }
    buf_.reserve(buf_.size() + (n_elems * tuple_inputs_.size() * sizeof(Elem)));
    for (int bi = 0; bi < n_elems; bi++) {
      for (auto& in : tuple_inputs_) {
        for (int j = 0; j < sizeof(Elem); j++) {
          auto byte = in.buf[j][bi];
          // auto encoded = byte ^ in.old[j];
          in.old[j] = byte;
          // buf_.push_back(in.buf[j][bi]);
          buf_.push_back(byte);
        }
      }
    }
    next_pos_ = 0;
  }
  const std::string name_;
  std::unique_ptr<ComboInputStream> combo_in_;
  std::vector<ElemInput> tuple_inputs_;
  std::vector<uint8_t> buf_;
  size_t next_pos_ = 0;
};

}  // namespace fpcompress
