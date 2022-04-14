#pragma once

#include <memory>

#include "input_stream.h"
#include "output_stream.h"

namespace fpcompress {
// Creates a new compressing output stream. If level >= 0, sets the compression
// level to the given value. If level<0, uses the default compression level.
extern std::unique_ptr<OutputStream> NewZstdCompressor(
    int level, std::unique_ptr<OutputStream> out);

// Creates a new decompressing input stream.
extern std::unique_ptr<InputStream> NewZstdDecompressor(
    std::unique_ptr<InputStream> in);
}  // namespace fpcompress
