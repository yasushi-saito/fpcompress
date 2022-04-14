#include "file_stream.h"

#include <fcntl.h>
#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <vector>
namespace fpcompress {
namespace {
class FileOutputStream : public OutputStream {
 private:
  const std::string path_;
  int fd_;

 public:
  explicit FileOutputStream(std::string path) : path_(std::move(path)) {
    fd_ = ::open(path_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    PCHECK(fd_ >= 0) << "create " << path_;
  }
  ~FileOutputStream() { PCHECK(::close(fd_) == 0) << "close " << path_; }

  std::string Name() const override { return path_; }

  void Append(std::span<const uint8_t> data) override {
    for (;;) {
      auto n = ::write(fd_, data.data(), data.size());
      if (n < 0 && errno == EINTR) continue;
      PCHECK(n == static_cast<ssize_t>(data.size())) << "write " << path_;
      break;
    }
  }
  void Flush() override {}
};

class FileInputStream : public InputStream {
 private:
  const std::string path_;
  int fd_;
  std::vector<uint8_t> buf_;

 public:
  explicit FileInputStream(std::string path) : path_(std::move(path)) {
    fd_ = ::open(path_.c_str(), O_RDONLY);
    PCHECK(fd_ >= 0) << "open " << path_;
  }
  ~FileInputStream() { PCHECK(::close(fd_) == 0) << "close " << path_; }

  std::string Name() const override { return path_; }

  std::span<const uint8_t> Read(int bytes) override {
    if (static_cast<int>(buf_.size()) < bytes) {
      buf_.resize(bytes + 256);
    }
    if (bytes == 0) return std::span<const uint8_t>();

    auto* out = &buf_[0];
    auto remaining = bytes;
    while (remaining > 0) {
      auto n = ::read(fd_, out, remaining);
      if (n < 0) {
        if (errno == EINTR) continue;
        PLOG(FATAL) << "read " << path_;
      }
      if (n == 0) break;
      out += n;
      remaining -= n;
    }
    return std::span<const uint8_t>(&buf_[0], bytes - remaining);
  }
};
}  // namespace
}  // namespace fpcompress

std::unique_ptr<fpcompress::OutputStream> fpcompress::NewFileOutputStream(
    const std::string& path) {
  return std::make_unique<FileOutputStream>(path);
}

std::unique_ptr<fpcompress::InputStream> fpcompress::NewFileInputStream(
    const std::string& path) {
  return std::make_unique<FileInputStream>(path);
}
