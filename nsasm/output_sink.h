#ifndef NSASM_OUTPUT_SINK_H_
#define NSASM_OUTPUT_SINK_H_

#include "absl/types/span.h"
#include "nsasm/address.h"
#include "nsasm/error.h"

namespace nsasm {

// General interface for writing assembled instructions to an address map.
class OutputSink {
 public:
  virtual ~OutputSink() = default;

  // Write the given string of bytes, starting at the given address.
  // Implementations may return errors on failure cases (writes to a bad
  // address, etc.)
  virtual ErrorOr<void> Write(nsasm::Address address,
                              absl::Span<const std::uint8_t> data) = 0;
};

}  // namespace nsasm

#endif  // NSASM_OUTPUT_SINK_H_
