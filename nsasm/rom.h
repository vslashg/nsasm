#ifndef NSASM_ROM_H_
#define NSASM_ROM_H_

#include <cstdint>
#include <string>

#include "nsasm/error.h"
#include "nsasm/output_sink.h"

namespace nsasm {

enum Mapping {
  kLoRom,    // modes $20 and $30
  kHiRom,    // modes $21 and $31
  kExHiRom,  // modes $25 and $35
};

// Convert an address in the 24-bit SNES address space to an offset into
// cartridge ROM.
//
// The `mapping` argument indicates the mapping from SNES to ROM address space
// implemented by the cart.
//
// Returns nullopt if snes_address is out of range, or if it maps to an address
// intercepted by the SNES (for work ram or memory-mapped registers, say.)
ErrorOr<size_t> SnesToROMAddress(nsasm::Address snes_address, Mapping mapping);

class RomOverwriter;

// Representation of a SNES ROM, presumably loaded from disk.
class Rom {
 public:
  Rom(Mapping mapping_mode, std::string path, std::vector<uint8_t> header,
      std::vector<uint8_t> data)
      : mapping_mode_(mapping_mode),
        path_(std::move(path)),
        header_(std::move(header)),
        data_(std::move(data)) {}

  // Returns `length` bytes of program data, starting at `address`, incrementing
  // addresses with the same logic as `AddToPC()` above.
  //
  // Returns nullopt instead if given an out-of-range read region.
  ErrorOr<std::vector<uint8_t>> Read(nsasm::Address address, int length) const;

  ErrorOr<int> ReadByte(nsasm::Address address) const;
  ErrorOr<int> ReadWord(nsasm::Address address) const;
  ErrorOr<int> ReadLong(nsasm::Address address) const;

  const std::string& path() const { return path_; }

 private:
  friend class RomOverwriter;
  Mapping mapping_mode_;
  std::string path_;
  std::vector<uint8_t> header_;
  std::vector<uint8_t> data_;
};

ErrorOr<Rom> LoadRomFile(const std::string& path);

// Wraps a SNES ROM and acts as an output sink.  Returns an error if any data
// written does not match what already exists in a ROM.  This is intended for
// testing and disassembly validation purposes.
class RomIdentityTest : public OutputSink {
 public:
  RomIdentityTest(const Rom* rom) : rom_(rom) {}

  ErrorOr<void> Write(nsasm::Address address,
                      absl::Span<const std::uint8_t> data) override;

 private:
  const Rom* rom_;
};

// Sink for assembling data over an existing ROM file.
class RomOverwriter : public OutputSink {
 public:
  RomOverwriter(const Rom* rom) : rom_(rom), data_(rom->data_) {}

  ErrorOr<void> Write(nsasm::Address address,
                      absl::Span<const std::uint8_t> data) override;

  ErrorOr<void> CreateFile(const std::string& path) const;

 private:
  const Rom* rom_;  
  std::vector<uint8_t> data_;
};

}  // namespace nsasm

#endif  // NSASM_ROM_H_