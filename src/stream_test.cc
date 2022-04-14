#include <fcntl.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>

#include "absl/random/random.h"
#include "absl/strings/str_format.h"
#include "combo_stream.h"
#include "file_stream.h"
#include "memory_stream.h"
#include "zstd_stream.h"

const char* kTestPath = "/etc/bash.bashrc";

std::string ReadFile(const std::string& path) {
  int fd = open(path.c_str(), O_RDONLY);
  PCHECK(fd >= 0);

  struct stat st;
  PCHECK(fstat(fd, &st) == 0);
  std::string data;
  data.resize(st.st_size);
  int n = read(fd, &data[0], data.size());
  CHECK_EQ(n, data.size());
  PCHECK(close(fd) == 0);
  return data;
}

template <typename T>
std::string ToString(const T& data) {
  std::string s;
  for (auto ch : data) {
    s.append(1, static_cast<char>(ch));
  }
  return s;
}

namespace fpcompress {

std::vector<uint8_t> RandomData() {
  absl::BitGen bitgen;
  size_t len = absl::Uniform(bitgen, 1, 2048);
  std::vector<uint8_t> s;
  for (int i = 0; i < len; i++) {
    s.push_back(absl::Uniform(bitgen, 'a', 'z'));
  }
  return s;
}

void WriteRandomData(std::span<const uint8_t> data, OutputStream* out) {
  absl::BitGen bitgen;
  while (!data.empty()) {
    int len = absl::Uniform<int>(bitgen, 1, data.size());
    out->Append(data.subspan(0, len));
    data = data.subspan(len);
  }
}

void ReadRandomData(std::span<const uint8_t> want, InputStream* in) {
  absl::BitGen bitgen;
  LOG(INFO) << "Read: " << want.size() << "b";
  while (!want.empty()) {
    int len = absl::Uniform<int>(bitgen, 1, want.size());
    LOG(INFO) << "Read: len=" << len;
    auto got = in->Read(len);
    ASSERT_EQ(got.size(), len);
    ASSERT_EQ(ToString(got), ToString(want.subspan(0, len)));
    want = want.subspan(len);
  }
  auto got = in->Read(1);
  ASSERT_EQ(got.size(), 0);
}

TEST(StreamTest, FileInput) {
  auto want = ReadFile(kTestPath);
  {
    auto in = NewFileInputStream(kTestPath);
    auto got = in->Read(want.size());
    ASSERT_EQ(got.size(), want.size());
    ASSERT_EQ(std::string(reinterpret_cast<const char*>(&got[0]), got.size()),
              want);
  }
  {
    auto in = NewFileInputStream(kTestPath);
    auto got = in->Read(10);
    ASSERT_EQ(got.size(), 10);
    ASSERT_EQ(std::string(reinterpret_cast<const char*>(&got[0]), got.size()),
              want.substr(0, 10));
    got = in->Read(0xffffff);
    ASSERT_EQ(got.size(), want.size() - 10);
    ASSERT_EQ(std::string(reinterpret_cast<const char*>(&got[0]), got.size()),
              want.substr(10));
  }
}

TEST(StreamTest, Memory) {
  for (int rep = 0; rep < 10; rep++) {
    auto data = RandomData();
    std::vector<uint8_t> got;
    {
      auto out = NewMemoryOutputStream(&got);
      WriteRandomData(data, out.get());
      out->Flush();
    }
    ASSERT_EQ(ToString(data), ToString(got));
    {
      auto in = NewMemoryInputStream(got);
      ReadRandomData(data, in.get());
    }
  }
}

TEST(StreamTest, FileInOut) {
  const char* path = "/tmp/fileinout";
  for (int rep = 0; rep < 10; rep++) {
    auto data = RandomData();
    {
      auto out = NewFileOutputStream(path);
      WriteRandomData(data, out.get());
      out->Flush();
    }
    {
      auto in = NewFileInputStream(path);
      ReadRandomData(data, in.get());
    }
  }
}

TEST(StreamTest, Zstd) {
  int n_lt = 0;
  int n_gt = 0;
  int n_eq = 0;
  for (int rep = 0; rep < 10; rep++) {
    auto data = RandomData();
    std::vector<uint8_t> got;
    {
      auto out = NewZstdCompressor(-1, NewMemoryOutputStream(&got));
      WriteRandomData(data, out.get());
      out->Flush();
    }
    if (got.size() < data.size())
      n_lt++;
    else if (got.size() > data.size())
      n_gt++;
    else
      n_eq++;
    {
      auto in = NewZstdDecompressor(NewMemoryInputStream(got));
      ReadRandomData(data, in.get());
    }
  }
  ASSERT_LT(n_gt, n_lt);
}

TEST(StreamTest, Combo) {
  absl::BitGen bitgen;

  for (int rep = 0; rep < 10; rep++) {
    const int n_shards = absl::Uniform(bitgen, 1, 5);
    const int n_flushes = absl::Uniform(bitgen, 1, 5);
    using Blob = std::vector<uint8_t>;

    std::vector<std::vector<Blob>> data(n_flushes);
    for (int i = 0; i < n_flushes; i++) {
      for (int j = 0; j < n_shards; j++) {
        data[i].push_back(RandomData());
      }
    }
    std::vector<uint8_t> got;
    {
      std::vector<ComboOutputStream::OutputSpec> spec(n_shards);
      for (int j = 0; j < n_shards; j++) {
        spec[j].name = absl::StrFormat("block%d", j);
        spec[j].factory = [](int, const std::string&,
                             std::unique_ptr<OutputStream> out)
            -> std::unique_ptr<OutputStream> { return out; };
      }
      auto combo_out =
          std::make_unique<ComboOutputStream>(NewMemoryOutputStream(&got));
      auto outputs =
          combo_out->Init(std::span<const ComboOutputStream::OutputSpec>(spec));

      for (int i = 0; i < n_flushes; i++) {
        combo_out->NextBlock();
        for (int j = 0; j < n_shards; j++) {
          WriteRandomData(data[i][j], outputs[j].get());
        }
      }
      // Must close the combo_out before deleting outputs.
      combo_out.reset();
    }
    {
      std::vector<ComboOutputStream::OutputSpec> spec(n_shards);
      auto combo_in =
          std::make_unique<ComboInputStream>(NewMemoryInputStream(got));
      auto inputs = combo_in->Init(
          [](int, const std::string&,
             std::unique_ptr<InputStream> in) -> std::unique_ptr<InputStream> {
            return in;
          });
      const auto& header = combo_in->Header();
      ASSERT_EQ(header.output_size(), n_shards);
      for (int j = 0; j < n_shards; j++) {
        ASSERT_EQ(header.output(j).name(), absl::StrFormat("block%d", j));
      }

      for (int i = 0; i < n_flushes; i++) {
        combo_in->NextBlock();
        for (int j = 0; j < n_shards; j++) {
          ReadRandomData(data[i][j], inputs[j].get());
        }
      }
      ASSERT_EQ(combo_in->NextBlock(), nullptr);
    }
  }
}

}  // namespace fpcompress

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  FLAGS_logtostderr = true;
  google::InitGoogleLogging("test");
  return RUN_ALL_TESTS();
}
