#include "zstd_stream.h"

#include <glog/logging.h>
#include <zstd.h>

#include <memory>

#define ZSTD_CHECK(op)                                   \
  [&]() -> size_t {                                      \
    auto err = (op);                                     \
    CHECK(!ZSTD_isError(err)) << ZSTD_getErrorName(err); \
    return err;                                          \
  }()

namespace fpcompress {
namespace {
std::span<const uint8_t> ToSpan(const void* buf, size_t n) {
  return std::span<const uint8_t>(static_cast<const uint8_t*>(buf), n);
}

class ZstdCompressor : public OutputStream {
 public:
  explicit ZstdCompressor(int level, std::unique_ptr<OutputStream> out)
      : out_(std::move(out)) {
    zstd_ = ZSTD_createCStream();
    if (level >= 0) {
      ZSTD_CHECK(ZSTD_CCtx_setParameter(zstd_, ZSTD_c_compressionLevel, level));
    }
    outbuf_.resize(kBufSize);
  }

  ~ZstdCompressor() {
    CHECK_EQ(ZSTD_freeCStream(zstd_), static_cast<size_t>(0));
  }

  std::string Name() const { return "zstdcompressor"; }

  void Append(std::span<const uint8_t> src) override {
    if (outbuf_.size() < src.size()) {
      outbuf_.resize(src.size() + 256);
    }
    ZSTD_inBuffer in = {.src = src.data(), .size = src.size(), .pos = 0};
    for (;;) {
      auto org_in_pos = in.pos;
      ZSTD_outBuffer out = {
          .dst = &outbuf_[0], .size = outbuf_.size(), .pos = 0};
      ZSTD_CHECK(ZSTD_compressStream2(zstd_, &out, &in, ZSTD_e_continue));
      CHECK_GT(in.pos, org_in_pos);
      if (out.pos > 0) {
        out_->Append(ToSpan(out.dst, out.pos));
        out.pos = 0;
      }
      if (in.pos == in.size) break;
    }
  }

  void Flush() override {
    ZSTD_outBuffer out = {.dst = &outbuf_[0], .size = outbuf_.size(), .pos = 0};
    ZSTD_CHECK(ZSTD_flushStream(zstd_, &out));
    if (out.pos > 0) {
      out_->Append(ToSpan(out.dst, out.pos));
      out.pos = 0;
    }
  }

 private:
  static constexpr int kBufSize = 4 << 20;
  ZSTD_CStream* zstd_;
  std::vector<uint8_t> outbuf_;
  std::unique_ptr<OutputStream> out_;
};

class ZstdDecompressor : public InputStream {
 public:
  ZstdDecompressor(std::unique_ptr<InputStream> in)
      : zstd_(ZSTD_createDStream()),
        in_(std::move(in)),
        inbuf_({.src = nullptr, .size = 0, .pos = 0}) {}
  ~ZstdDecompressor() {
    CHECK_EQ(ZSTD_freeDStream(zstd_), static_cast<size_t>(0));
  }

  std::string Name() const { return "zstddecompressor"; }

  std::span<const uint8_t> Read(int bytes) override {
    static constexpr int kInbufSize = 4 << 20;
    if (outbuf_.capacity() < bytes) {
      outbuf_.reserve(bytes + 256);
    }
    outbuf_.resize(bytes);
    ZSTD_outBuffer out = {.dst = &outbuf_[0], .size = outbuf_.size(), .pos = 0};
    while (out.pos < out.size) {
      CHECK_LE(inbuf_.pos, inbuf_.size);
      if (inbuf_.pos == inbuf_.size) {
        const auto src = in_->Read(kInbufSize);
        inbuf_ = ZSTD_inBuffer{.src = src.data(), .size = src.size(), .pos = 0};
      }
      auto old_pos = out.pos;
      ZSTD_CHECK(ZSTD_decompressStream(zstd_, &out, &inbuf_));
      if (old_pos == out.pos && inbuf_.pos == 0) {
        // EOF on the input, and the output didn't grow. We are at the end of
        // the stream.
        break;
      }
    }
    return ToSpan(out.dst, out.pos);
  }

 private:
  ZSTD_DStream* const zstd_;
  std::unique_ptr<InputStream> in_;
  ZSTD_inBuffer inbuf_;
  std::vector<uint8_t> outbuf_;
};
}  // namespace
}  // namespace fpcompress

std::unique_ptr<fpcompress::OutputStream> fpcompress::NewZstdCompressor(
    int level, std::unique_ptr<OutputStream> out) {
  return std::make_unique<ZstdCompressor>(level, std::move(out));
}

std::unique_ptr<fpcompress::InputStream> fpcompress::NewZstdDecompressor(
    std::unique_ptr<InputStream> in) {
  return std::make_unique<ZstdDecompressor>(std::move(in));
}
