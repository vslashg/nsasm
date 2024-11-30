#ifndef NSASM_MEMORY_H_
#define NSASM_MEMORY_H_

#include <cstdint>

#include "absl/types/span.h"
#include "nsasm/address.h"
#include "nsasm/error.h"

namespace nsasm {

// General interface for reading bytes from a source during disassembly.
class InputSource {
 public:
  virtual ~InputSource() = default;

  // Returns a string representing the source of this data, as a file path.
  // Used in the formation of error messages.
  virtual std::string Path() const = 0;

  // Returns `length` bytes of program data, starting at `address`.  This
  // should read memory in the same way the PC is advance (wrapping at banks).
  //
  // Returns nullopt instead if given an out-of-range read region.
  virtual ErrorOr<std::vector<uint8_t>> Read(nsasm::Address address,
                                             int length) const = 0;

  // Helper functions to read 1, 2, or 3-byte long little-endian values from
  // an address.
  ErrorOr<int> ReadByte(nsasm::Address address) const;
  ErrorOr<int> ReadWord(nsasm::Address address) const;
  ErrorOr<int> ReadLong(nsasm::Address address) const;
};

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

#endif  // NSASM_MEMORY_H_