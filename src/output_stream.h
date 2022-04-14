#pragma once
#include <span>

namespace fpcompress {
class OutputStream {
 public:
  virtual ~OutputStream() = default;

  // Returns a human-readable name of this compressor. For logging only.
  virtual std::string Name() const = 0;
  // Compress the given sequence of bytes.
  virtual void Append(std::span<const uint8_t> data) = 0;
  virtual void Flush() = 0;
};
}  // namespace fpcompress
