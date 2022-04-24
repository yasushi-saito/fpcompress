#include "combo_stream.h"

#include <glog/logging.h>

#include <vector>

#include "bit_cast.h"
#include "memory_stream.h"

namespace fpcompress {
class ComboOutputStream::Shard {
 public:
  explicit Shard(ComboOutputStream* parent) : parent_(parent) {
    buf_.reserve(kInitialBufSize);
  }
  std::unique_ptr<OutputStream> Init(int output_index, const OutputSpec& spec) {
    auto out =
        spec.factory(output_index, spec.name, NewMemoryOutputStream(&buf_));
    out_ = out.get();
    return out;
  }

  OutputStream* Output() const { return out_; }
  std::vector<uint8_t>* MutableBuf() { return &buf_; }

 private:
  friend class ComboOutputStream;
  static constexpr int kInitialBufSize = 4 << 20;
  ComboOutputStream* const parent_;
  std::vector<uint8_t> buf_;
  OutputStream* out_;
};  // namespace fpcompress

ComboOutputStream::ComboOutputStream(std::unique_ptr<OutputStream> combo_out)
    : combo_out_(std::move(combo_out)) {}

std::vector<std::unique_ptr<OutputStream>> ComboOutputStream::Init(
    std::span<const OutputSpec> outputs) {
  std::vector<std::unique_ptr<OutputStream>> outs;
  int index = 0;
  for (const auto& spec : outputs) {
    auto* out = header_.add_output();
    *out->mutable_name() = spec.name;
    shards_.push_back(std::make_unique<Shard>(this));
    outs.push_back(shards_.back()->Init(index, spec));
    index++;
  }
  return outs;
}

ComboOutputStream::~ComboOutputStream() { NextBlock(); }

void ComboOutputStream::WriteProto(const google::protobuf::Message& p) {
  const auto data = p.SerializeAsString();
  VLOG(1) << "Write proto: " << p.ShortDebugString();
  auto header = ToBytes(data.size());
  combo_out_->Append(std::span<const uint8_t>(header.data(), header.size()));
  combo_out_->Append(std::span<const uint8_t>(
      reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

void ComboOutputStream::NextBlock() {
  if (!started_) {
    WriteProto(header_);
    started_ = true;
  } else {
    ComboBlockProto block_header;
    for (auto& shard : shards_) {
      shard->Output()->Flush();
      auto* out = block_header.add_output();
      out->set_len(shard->MutableBuf()->size());
    }
    WriteProto(block_header);
    for (auto& shard : shards_) {
      combo_out_->Append(std::span<const uint8_t>(*shard->MutableBuf()));
      shard->MutableBuf()->clear();
    }
  }
}

ComboInputStream::ComboInputStream(std::unique_ptr<InputStream> in)
    : in_(std::move(in)) {}

ComboInputStream::~ComboInputStream() {}

class ComboInputStream::Shard {
 public:
  explicit Shard(ComboInputStream* parent) : parent_(parent) {}

  std::unique_ptr<InputStream> Init(int index, std::string name,
                                    Factory factory) {
    auto in = factory(index, name, std::make_unique<Stream>(this));
    in_ = in.get();
    return in;
  }

  InputStream* Input() { return in_; }

  void StartBlock(std::span<const uint8_t> data) {
    buf_.resize(data.size());
    std::copy(data.begin(), data.end(), buf_.begin());
    data_ = buf_;
  }

 private:
  class Stream : public InputStream {
   private:
    Shard* const parent_;

   public:
    explicit Stream(Shard* parent) : parent_(parent) {}
    std::string Name() const { return "mem"; }
    std::span<const uint8_t> Read(int bytes) {
      bytes = std::min<int>(bytes, parent_->data_.size());
      auto r = parent_->data_.subspan(0, bytes);
      parent_->data_ = parent_->data_.subspan(bytes);
      return r;
    }
  };

  friend class ComboOutputStream;
  static constexpr int kInitialBufSize = 4 << 20;
  const std::string name_;
  InputStream* in_;
  std::vector<uint8_t> buf_;
  std::span<const uint8_t> data_;
  ComboInputStream* const parent_;
};

const ComboBlockProto* ComboInputStream::NextBlock() {
  if (!ReadProto(&block_)) return nullptr;
  CHECK_EQ(block_.output_size(), header_.output_size());

  for (int i = 0; i < block_.output_size(); i++) {
    const auto& out = block_.output(i);
    auto* shard = inputs_[i].get();
    const auto data = in_->Read(out.len());
    CHECK_EQ(data.size(), out.len());
    shard->StartBlock(data);
  }
  return &block_;
}

bool ComboInputStream::ReadProto(google::protobuf::Message* p) {
  auto len_data = in_->Read(sizeof(size_t));
  if (len_data.empty()) return false;
  auto len = FromBytes<size_t>(len_data);
  auto bytes = in_->Read(len);
  CHECK_EQ(bytes.size(), len);
  p->ParseFromArray(bytes.data(), bytes.size());
  return true;
}

std::vector<std::unique_ptr<InputStream>> ComboInputStream::Init(
    Factory factory) {
  CHECK(ReadProto(&header_)) << "read header: " << in_->Name();
  inputs_.reserve(header_.output_size());

  std::vector<std::unique_ptr<InputStream>> inputs;

  int index = 0;
  for (const auto& out : header_.output()) {
    inputs_.push_back(std::make_unique<Shard>(this));
    index++;
    inputs.push_back(inputs_.back()->Init(index, out.name(), factory));
  }
  return inputs;
}
}  // namespace fpcompress
