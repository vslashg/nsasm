#ifndef TEST_TEST_ROM_H_
#define TEST_TEST_ROM_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "nsasm/assembler.h"
#include "nsasm/error.h"
#include "nsasm/rom.h"

namespace nsasm {

// A run of bytes to place in a synthetic ROM, at a SNES address.
struct RomContents {
  Address address;
  std::vector<std::uint8_t> bytes;
};

// Builds an in-memory Rom for tests.  Each byte run is placed via
// SnesToROMAddress() for the given mapping; the ROM image is sized to cover
// the highest mapped offset (rounded up to a 0x8000 multiple) and zero-filled
// elsewhere.  No file I/O is involved.
ErrorOr<std::unique_ptr<Rom>> MakeTestRom(
    Mapping mapping, const std::vector<RomContents>& contents,
    std::string path = "test.rom");

// The result of assembling sources into a synthetic ROM: the ROM image plus
// the Assembler, for post-assembly queries (JumpTargets(), etc.) used to seed
// disassembly.
struct AssembledRom {
  std::unique_ptr<Rom> rom;
  Assembler assembler;
};

// Runs the real assembly pipeline over the given .asm file contents and
// materializes the output as an in-memory Rom with the given mapping.
ErrorOr<AssembledRom> AssembleToRom(
    const std::vector<std::string_view>& asm_contents,
    Mapping mapping = kLoRom);

inline ErrorOr<AssembledRom> AssembleToRom(std::string_view asm_contents,
                                           Mapping mapping = kLoRom) {
  return AssembleToRom(std::vector<std::string_view>{asm_contents}, mapping);
}

}  // namespace nsasm

#endif  // TEST_TEST_ROM_H_
