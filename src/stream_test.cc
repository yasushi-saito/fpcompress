#include <fcntl.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <random>

#include "absl/strings/str_format.h"
#include "columnar_stream.h"
#include "combo_stream.h"
#include "const.h"
#include "file_stream.h"
#include "id_encoder.h"
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

class StreamTest : public testing::Test {
 private:
  int old_buf_size_ = -1;
  std::mt19937 rnd_;

 public:
  void SetUp() override {
    old_buf_size_ = kBufSize;
    rnd_ = std::mt19937(100);
  }
  void TearDown() override { kBufSize = old_buf_size_; }

  // Generate a random number in range [min, max].
  int UniformRandom(int min, int max) {
    auto d = std::uniform_int_distribution<int>(min, max);
    return d(rnd_);
  }

  std::vector<uint8_t> RandomData() {
    const int len = UniformRandom(1, 2048);
    std::vector<uint8_t> s;
    for (int i = 0; i < len; i++) {
      s.push_back(UniformRandom('a', 'z'));
    }
    return s;
  }

  void WriteRandomData(std::span<const uint8_t> data, OutputStream* out) {
    while (!data.empty()) {
      int len = UniformRandom(1, data.size());
      out->Append(data.subspan(0, len));
      data = data.subspan(len);
    }
  }

  void ReadRandomData(int seed, std::span<const uint8_t> want,
                      InputStream* in) {
    static int nn = 0;
    VLOG(0) << "Read: " << want.size() << "b";
    while (!want.empty()) {
      int len = UniformRandom(1, want.size());
      nn++;
      VLOG(0) << "Read: len=" << len << " n=" << nn;
      auto got = in->Read(len);
      CHECK_EQ(got.size(), len);
      ASSERT_EQ(ToString(got), ToString(want.subspan(0, len)));
      want = want.subspan(len);
    }
    auto got = in->Read(1);
    ASSERT_EQ(got.size(), 0);
  }
};

TEST_F(StreamTest, FileInput) {
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

TEST_F(StreamTest, Memory) {
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
      ReadRandomData(rep + 200, data, in.get());
    }
  }
}

TEST_F(StreamTest, FileInOut) {
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
      ReadRandomData(rep + 200, data, in.get());
    }
  }
}

TEST_F(StreamTest, Zstd) {
  int n_lt = 0;
  int n_gt = 0;
  int n_eq = 0;
  for (int rep = 0; rep < 10; rep++) {
    kBufSize = UniformRandom(100, 200);
    auto data = RandomData();
    std::vector<uint8_t> got;
    {
      auto out = NewZstdCompressor(-1, NewMemoryOutputStream(&got));
      WriteRandomData(data, out.get());
      out->Flush();
    }
    LOG(INFO) << "before=" << got.size() << " after=" << data.size();
    if (got.size() < data.size()) {
      n_lt++;
    } else if (got.size() > data.size()) {
      n_gt++;
    } else {
      n_eq++;
    }
    {
      auto in = NewZstdDecompressor(NewMemoryInputStream(got));
      ReadRandomData(rep + 200, data, in.get());
    }
  }
  ASSERT_LT(n_gt, n_lt);
}

TEST_F(StreamTest, Columnar) {
  std::vector<uint8_t> got;
  using Elem = uint32_t;
  const int tuple_size = UniformRandom(1, 4);
  auto data = RandomData();
  if (const auto rem = data.size() % (sizeof(Elem) * tuple_size); rem != 0) {
    data.resize(data.size() - rem);
  }
  {
    auto out = std::make_unique<ColumnarOutputStream<IdEncoder<Elem>>>(
        "test", tuple_size,
        [](int, const std::string&, std::unique_ptr<OutputStream> out) {
          return out;
        },
        NewMemoryOutputStream(&got));
    out->Append(data);
  }
  {
    auto in = std::make_unique<ColumnarInputStream<IdEncoder<Elem>>>(
        "test", tuple_size,
        [](int, const std::string&, std::unique_ptr<InputStream> in) {
          return in;
        },
        NewMemoryInputStream(got));
    ReadRandomData(100, data, in.get());
  }
}

TEST_F(StreamTest, Combo) {
  for (int rep = 0; rep < 10; rep++) {
    const int n_shards = UniformRandom(1, 5);
    const int n_flushes = UniformRandom(1, 5);
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
          ReadRandomData(rep + 200 + i + j, data[i][j], inputs[j].get());
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
