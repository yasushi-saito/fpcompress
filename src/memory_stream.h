#pragma once

#include <memory>
#include <vector>

#include <span>
#include "input_stream.h"
#include "output_stream.h"

namespace fpcompress {
// Creates a stream that writes to the given vector. The vector
// is owned by the caller and it must outlive the stream.
std::unique_ptr<OutputStream> NewMemoryOutputStream(std::vector<uint8_t>* data);

// Creates a stream that reads from the given array.  The array is owned by the
// caller and it must outlive the stream.
std::unique_ptr<InputStream> NewMemoryInputStream(
    std::span<const uint8_t> data);
}  // namespace fpcompress
