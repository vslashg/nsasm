#ifndef NSASM_ROM_H_
#define NSASM_ROM_H_

#include "absl/types/optional.h"

#include <cstdint>

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
absl::optional<int> SnesToROMAddress(int snes_address, Mapping mapping);

// Add the given offset to an address.  Adds do not carry over into the bank
// word.  (In other words, byte 2 does not carry into byte 3.  This is a weird
// consequence of the 24 bit program counter being split between the PC and K
// registers in the 65816.)
constexpr int AddToPC(int address, int offset) {
  return (address & 0xff0000) | ((address + offset) & 0xffff);
}

// Representation of a SNES ROM, presumably loaded from disk.
class Rom {
 public:
  Rom(Mapping mapping_mode, std::vector<uint8_t> data)
      : mapping_mode_(mapping_mode), data_(std::move(data)) {}

  // Returns `length` bytes of program data, starting at `address`, incrementing
  // addresses with the same logic as `AddToPC()` above.
  //
  // Returns nullopt instead if given an out-of-range read region.
  absl::optional<std::vector<uint8_t>> Read(int address, int length);

 private:
  Mapping mapping_mode_;
  std::vector<uint8_t> data_;
};

absl::optional<Rom> LoadRomFile(const std::string& path);

}  // namespace nsasm

#endif  // NSASM_ROM_H_