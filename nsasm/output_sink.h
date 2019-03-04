#ifndef NSASM_OUTPUT_SINK_H
#define NSASM_OUTPUT_SINK_H

#include "absl/types/span.h"
#include "nsasm/error.h"

namespace nsasm {

// General interface for writing assembled instructions to an address map.
class OutputSink {
 public:
  // Write the given string of bytes, starting at the given address.
  // Implementations may return errors on failure cases (writes to a bad
  // address, etc.)
  virtual ErrorOr<void> Write(int address,
                              absl::Span<const std::uint8_t> data) = 0;
};

}  // namespace nsasm

#endif  // NSASM_OUTPUT_SINK_H
