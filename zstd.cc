#include "zstd.h"

#include "type.h"

#include "zstd.h"
#include <memory>

class ZstdCompressor : public Compressor {
  std::string Name() const { return "zstd"; }
  Bytes Compress(const Bytes &src) {
    const size_t max_size = ZSTD_compressBound(src.size());
    Bytes dest;
    dest.resize(max_size);
    size_t dest_size =
        ZSTD_compress(dest.data(), dest.size(), src.data(), src.size(), 6);
    dest.resize(dest_size);
    return dest;
  }
  Bytes Decompress(const Bytes &src) {
    const size_t dest_size = ZSTD_getDecompressedSize(src.data(), src.size());
    Bytes dest;
    dest.resize(dest_size);
    size_t n =
        ZSTD_decompress(dest.data(), dest.size(), src.data(), src.size());
    CHECK(n == dest_size);
    return dest;
  }
};

std::unique_ptr<Compressor> NewZstd() {
  return std::unique_ptr<ZstdCompressor>(new ZstdCompressor());
}
