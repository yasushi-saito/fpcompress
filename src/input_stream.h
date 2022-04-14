#pragma once
#include <span>
namespace fpcompress {
class InputStream {
 public:
  virtual ~InputStream() = default;

  // Returns a human-readable name of this compressor. For logging only.
  virtual std::string Name() const = 0;
  // Reads "bytes" bytes from the file. It returns the actual data read in
  // out. Contents in "out" is valid until the next call to Read. It always
  // tries to read exactly "bytes", unless it reaches EOF. It crashes the
  // process on any error.
  virtual std::span<const uint8_t> Read(int bytes) = 0;
};
}  // namespace fpcompress
