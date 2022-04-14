#include "memory_stream.h"

#include <algorithm>
namespace fpcompress {
namespace {
class MemoryInputStream : public InputStream {
 private:
  std::span<const uint8_t> data_;

 public:
  explicit MemoryInputStream(std::span<const uint8_t> data) : data_(data) {}
  std::string Name() const { return "mem"; }
  std::span<const uint8_t> Read(int bytes) {
    bytes = std::min<int>(bytes, data_.size());
    auto r = data_.subspan(0, bytes);
    data_ = data_.subspan(bytes);
    return r;
  }
};

class MemoryOutputStream : public OutputStream {
 private:
  std::vector<uint8_t>* const data_;

 public:
  explicit MemoryOutputStream(std::vector<uint8_t>* data) : data_(data) {}
  std::string Name() const { return "mem"; }
  void Append(std::span<const uint8_t> data) override {
    data_->insert(data_->end(), data.begin(), data.end());
  }
  void Flush() override {}
};
}  // namespace

std::unique_ptr<OutputStream> NewMemoryOutputStream(
    std::vector<uint8_t>* data) {
  return std::make_unique<MemoryOutputStream>(data);
}

std::unique_ptr<InputStream> NewMemoryInputStream(
    std::span<const uint8_t> data) {
  return std::make_unique<MemoryInputStream>(data);
}

}  // namespace fpcompress
