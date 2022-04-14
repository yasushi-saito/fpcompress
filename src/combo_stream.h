#pragma once
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "input_stream.h"
#include "output_stream.h"
#include "src/fpcompress.pb.h"
namespace fpcompress {

class ComboOutputStream {
 public:
  using Factory = std::function<std::unique_ptr<OutputStream>(
      int index, const std::string& name, std::unique_ptr<OutputStream>)>;
  struct OutputSpec {
    std::string name;
    Factory factory;
  };
  explicit ComboOutputStream(std::unique_ptr<OutputStream> combo_out);
  std::vector<std::unique_ptr<OutputStream>> Init(
      std::span<const OutputSpec> outputs);

  ~ComboOutputStream();
  void NextBlock();
  // Output shall be called exactly once per the given index.
  std::unique_ptr<OutputStream> Output(int index) const;

 private:
  class Shard;
  void WriteProto(const google::protobuf::Message& p);

  const std::unique_ptr<OutputStream> combo_out_;
  bool started_ = false;
  ComboHeaderProto header_;
  std::vector<std::unique_ptr<Shard>> shards_;
};

class ComboInputStream {
 public:
  using Factory = std::function<std::unique_ptr<InputStream>(
      int index, const std::string& name, std::unique_ptr<InputStream>)>;

  explicit ComboInputStream(std::unique_ptr<InputStream> in);
  ~ComboInputStream();

  // Get a reader for the next input stream in the current block. The call #K
  // (base zero) will return the stream for input Header().output(k).  The
  // caller must call NextInput() exactly N times before calling NextBlock
  // again, where N = header_.output_size().
  //
  // The returned stream remains owned by ComboInputStream.
  std::vector<std::unique_ptr<InputStream>> Init(Factory factory);

  const ComboHeaderProto& Header() const { return header_; };

  // Read the next block header.  It return null at EOF, and it crashes the
  // process on any error. NextBlock() must be called to read the first block in
  // the input.
  const ComboBlockProto* NextBlock();

 private:
  class Shard;
  bool ReadProto(google::protobuf::Message* p);

  const std::unique_ptr<InputStream> in_;
  ComboHeaderProto header_;
  ComboBlockProto block_;
  std::vector<std::unique_ptr<Shard>> inputs_;
};
}  // namespace fpcompress
