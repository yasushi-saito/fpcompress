#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "nlohmann/json.hpp"
#include "src/columnar.h"
#include "src/combo_stream.h"
#include "src/dfcm_predictor.h"
#include "src/file_stream.h"
#include "src/last_value_predictor.h"
#include "src/memory_stream.h"
#include "src/output_stream.h"
#include "src/stride_predictor.h"
#include "src/zero_predictor.h"
#include "src/zstd_stream.h"

enum DataType {
  F32 = 1,
  F64 = 2,
  U32 = 3,
  U64 = 4,
};

struct Dataset {
  std::string name;
  std::string path;
  DataType type;
  int dim;
};

struct Result {
  std::string compressor;
  std::vector<std::string> args;
  std::shared_ptr<Dataset> ds;
  size_t raw_size;
  size_t compressed_size;
  absl::Duration duration;
};

nlohmann::json DatasetToJson(std::shared_ptr<Dataset> ds) {
  nlohmann::json j;
  j["name"] = ds->name;
  j["path"] = ds->path;
  j["type"] = ds->type;
  j["dim"] = ds->dim;
  return j;
}

std::shared_ptr<Dataset> JsonToDataset(nlohmann::json j) {
  return std::make_shared<Dataset>(Dataset{
      .name = j["name"],
      .path = j["path"],
      .type = j["type"],
      .dim = j["dim"],
  });
}

nlohmann::json ResultToJson(const Result &r) {
  nlohmann::json j;
  j["compressor"] = r.compressor;
  j["args"] = nlohmann::json::array();
  for (const auto &arg : r.args) {
    j["args"].push_back(arg);
  }
  j["ds"] = DatasetToJson(r.ds);
  j["rawSize"] = r.raw_size;
  j["compressedSize"] = r.compressed_size;
  j["duration"] = absl::ToDoubleSeconds(r.duration);
  return j;
}

Result JsonToResult(nlohmann::json j) {
  Result r = {
      .compressor = j["compressor"],
      .ds = JsonToDataset(j["ds"]),
      .raw_size = j["rawSize"],
      .compressed_size = j["compressedSize"],
      .duration = absl::Nanoseconds(static_cast<double>(j["duration"]) * 10e9),
  };

  for (const auto &arg : j["args"]) {
    r.args.push_back(arg);
  }
  return r;
}

const char *kDataDir = "../data";

std::vector<std::shared_ptr<Dataset>> kDatasets = {
    std::make_shared<Dataset>(Dataset{
        .name = "rsim.f32",
        .path = absl::StrFormat("%s/fabian/rsim.f32", kDataDir),
        .type = F32,
        .dim = 1,
    }),
    std::make_shared<Dataset>(Dataset{
        .name = "rsim.f64",
        .path = absl::StrFormat("%s/fabian/rsim.f64", kDataDir),
        .type = F64,
        .dim = 1,
    }),
    std::make_shared<Dataset>(Dataset{
        .name = "astro_mhd.f32",
        .path = absl::StrFormat("%s/fabian/astro_mhd.f64", kDataDir),
        .type = F64,
        .dim = 1,
    }),
    std::make_shared<Dataset>(Dataset{
        .name = "astro_pt.f32",
        .path = absl::StrFormat("%s/fabian/astro_pt.f32", kDataDir),
        .type = F32,
        .dim = 3,
    }),
    std::make_shared<Dataset>(Dataset{
        .name = "astro_pt.f64",
        .path = absl::StrFormat("%s/fabian/astro_pt.f64", kDataDir),
        .type = F64,
        .dim = 3,
    }),
    std::make_shared<Dataset>(
        Dataset{.name = "wave.f32",
                .path = absl::StrFormat("%s/fabian/wave.f32", kDataDir),
                .type = F32,
                .dim = 2}),
    std::make_shared<Dataset>(
        Dataset{.name = "wave.f64",
                .path = absl::StrFormat("%s/fabian/wave.f64", kDataDir),
                .type = F64,
                .dim = 2}),
};

size_t FileSize(const std::string &path) {
  struct stat st;
  PCHECK(stat(path.c_str(), &st) == 0) << path;
  return st.st_size;
}

Result ZstdCommand(std::shared_ptr<Dataset> ds, int level) {
  auto start = absl::Now();
  const char *kOutPath = "/tmp/testout";
  std::string cmdline =
      absl::StrFormat("zstd -f%d -o %s -f %s", level, kOutPath, ds->path);
  LOG(INFO) << "Run: " << cmdline;
  CHECK_EQ(system(cmdline.c_str()), 0);
  auto end = absl::Now();
  return Result{
      .compressor = "zstd",
      .args = {absl::StrFormat("%d", level)},
      .ds = ds,
      .raw_size = FileSize(ds->path),
      .compressed_size = FileSize(kOutPath),
      .duration = end - start,
  };
}

Result ZstdColumnar(std::shared_ptr<Dataset> ds, int level) {
  std::vector<uint8_t> out;
  auto start = absl::Now();
  /*
  std::vector<fpcompress::ComboOutputStream::OutputSpec> spec(8);
  for (int i = 0; i < spec.size(); i++) {
    spec[i].name = absl::StrFormat("b%d", i);
    spec[i].factory = [level](int index, const std::string &name,
                              std::unique_ptr<fpcompress::OutputStream> out)
        -> std::unique_ptr<fpcompress::OutputStream> {
      return fpcompress::NewZstdCompressor(level, std::move(out));
    };
  }
  */
  /*
  auto combo_out = std::make_unique<fpcompress::ComboOutputStream>(
  fpcompress::NewMemoryOutputStream(&out));
      auto outputs = combo_out->Init(spec);*/
  auto column_out =
      std::make_unique<fpcompress::ColumnarOutputStream<ZeroPredictor<float>>>(
          "column", 1,
          [level](int index, const std::string &name,
                  std::unique_ptr<fpcompress::OutputStream> out)
              -> std::unique_ptr<fpcompress::OutputStream> {
            return fpcompress::NewZstdCompressor(level, std::move(out));
          },
          fpcompress::NewMemoryOutputStream(&out));
  auto file_in = fpcompress::NewFileInputStream(ds->path);
  const int kBufSize = 4 << 20;
  for (;;) {
    auto data = file_in->Read(kBufSize);
    if (data.empty()) break;
    column_out->Append(data);
  }
  column_out.reset();
  auto end = absl::Now();
  return Result{
      .compressor = "combo",
      .ds = ds,
      .raw_size = FileSize(ds->path),
      .compressed_size = out.size(),
      .duration = end - start,
  };
}

Result Zstd(std::shared_ptr<Dataset> ds, int level) {
  std::vector<uint8_t> out;
  auto start = absl::Now();
  auto zstd_out = fpcompress::NewZstdCompressor(
      level, fpcompress::NewMemoryOutputStream(&out));
  auto file_in = fpcompress::NewFileInputStream(ds->path);
  const int kBufSize = 4 << 20;
  for (;;) {
    auto data = file_in->Read(kBufSize);
    if (data.empty()) break;
    zstd_out->Append(data);
  }
  auto end = absl::Now();
  return Result{
      .compressor = "zstd",
      .args = {absl::StrFormat("%d", level)},
      .ds = ds,
      .raw_size = FileSize(ds->path),
      .compressed_size = out.size(),
      .duration = end - start,
  };
}

int main(int argc, char **argv) {
  auto r = ZstdColumnar(kDatasets[0], 3);
  LOG(INFO) << ResultToJson(r) << " ratio: "
            << static_cast<double>(r.raw_size) /
                   static_cast<double>(r.compressed_size);
  r = ZstdCommand(kDatasets[0], 3);
  LOG(INFO) << ResultToJson(r) << " ratio:"
            << static_cast<double>(r.raw_size) /
                   static_cast<double>(r.compressed_size);
  r = Zstd(kDatasets[0], 3);
  LOG(INFO) << ResultToJson(r) << " ratio: "
            << static_cast<double>(r.raw_size) /
                   static_cast<double>(r.compressed_size);
  return 0;
}
