#pragma once

#include <memory>

#include "input_stream.h"
#include "output_stream.h"

namespace fpcompress {
// Creates a stream that writes to the given file.  Existing contents, if any,
// are clobbered on open.
std::unique_ptr<OutputStream> NewFileOutputStream(const std::string& path);

// Creates a stream that reads from the given file. Crashes the process on
// error.
std::unique_ptr<InputStream> NewFileInputStream(const std::string& path);
}  // namespace fpcompress
