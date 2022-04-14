// Copyright 2020 Luminary Cloud, Inc. All Rights Reserved.

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "bit_cast.h"
#include "columnar.h"
#include "dfcm_predictor.h"
#include "fcntl.h"
#include "last_value_predictor.h"
#include "stride_predictor.h"
#include "zero_predictor.h"

#if 0
void PrintFloat64s(const Bytes &data, int tuple_size, int max_tuple) {
  CHECK(data.size() % (tuple_size * sizeof(double)) == 0);
  const int n_tuple =
      std::min<int>(max_tuple, data.size() / (tuple_size * sizeof(double)));
  for (int i = 0; i < n_tuple; i++) {
    std::cout << i << ": (";
    for (int j = 0; j < tuple_size; j++) {
      size_t off = (i * tuple_size + j) * sizeof(double);
      //const double value = *reinterpret_cast<const double *>(data.data() + off);
      //if (j > 0) {
      //std::cout << ", ";
      //}

      //const auto fp = ParseDouble(value);
      //std::cout << value << ", sign=" << int(fp.sign) << ",exp="<<fp.exp<<",dfrac=" << /*dfrac<<*/ ",frac="
      //<< std::hex<<fp.frac<<std::dec;
    }
    std::cout << ")\n";
  }
}
#endif

#if 0
Bytes ReadFile(const std::string &path) {
  int fd = open(path.c_str(), 0);
  CHECK(fd >= 0);
  struct stat st;
  CHECK(fstat(fd, &st) == 0);
  std::cerr << path << " size=" << st.st_size << "\n";
  std::vector<uint8_t> buf;
  buf.resize(st.st_size);
  CHECK(read(fd, buf.data(), st.st_size) == st.st_size);
  CHECK(close(fd) == 0);
  return buf;
}


void Points(const std::string &src_path) {
  const auto src = ReadFile(src_path);
  std::cout << "File: " << src_path << "\n";

  auto Run = [&src](std::unique_ptr<Compressor> c) {
    Bytes dest = c->Compress(src);
    std::cerr << c->Name() << ": orig=" << src.size() << " dest=" << dest.size()
              << " rate: " << (double)dest.size() / src.size() << "\n";
  };

#if 1
  Run(NewZstd());
#endif
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, StridePredictor<double>>("stride", 3)));
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, DfcmPredictor<double>>("dfcm", 3)));
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, LastValuePredictor<double>>("lastvalue",
                                                                 3)));
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, ZeroPredictor<double>>("zero", 3)));
}

void Scalars(const std::string &src_path) {
  const auto src = ReadFile(src_path);
  std::cout << "File: " << src_path << "\n";
  auto Run = [&src](std::unique_ptr<Compressor> c) {
    Bytes dest = c->Compress(src);
    std::cerr << c->Name() << ": orig=" << src.size() << " dest=" << dest.size()
              << " rate: " << (double)dest.size() / src.size() << "\n";
  };

#if 1
  Run(NewZstd());
#endif
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, StridePredictor<double>>("stride", 1)));
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, DfcmPredictor<double>>("dfcm", 1)));
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, LastValuePredictor<double>>("lastvalue",
                                                                 1)));
  Run(std::unique_ptr<Compressor>(
      new ColumnarCompressor<double, ZeroPredictor<double>>("zero", 1)));
}
#endif

int main(int argc, char **argv) {
  return 0;
}
