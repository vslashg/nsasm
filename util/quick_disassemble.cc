#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "nsasm/decode.h"
#include "nsasm/instruction.h"
#include "nsasm/rom.h"

#include <cstdint>
#include <cstdio>

// Test utility to exercise

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom> <snes-hex-address>\n\n"
      "Disassembles some code starting at the named offset.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    usage(argv[0]);
    return 0;
  }

  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.has_value()) {
    absl::PrintF("Failure reading ROM file.\n");
    return 1;
  }

  unsigned rd_address;
  if (!sscanf(argv[2], "%x", &rd_address)) {
    usage(argv[0]);
    return 1;
  }
  int pc = rd_address;

  // Assume we are in native mode, with one-byte A and X/Y.
  //
  // This is a wild guess but works for my ad-hoc testing purposes at the
  // moment.
  nsasm::FlagState flag_state(nsasm::B_off, nsasm::B_on, nsasm::B_on);
  while (true) {
    // read 4 bytes (max size of an instruction)
    auto data = rom->Read(pc, 4);
    if (!data.has_value()) {
      absl::PrintF("failed to read data...\n");
      return 0;
    }
    auto instruction = nsasm::Decode(*data, flag_state);
    if (!instruction.has_value()) {
      absl::PrintF("failed to decode...\n");
      return 0;
    }
    // Print a hexdecimal representation of the bytes read
    int bytes_read = InstructionLength(instruction->addressing_mode);
    for (int i = 0; i < 4; ++i) {
      if (i < bytes_read) {
        absl::PrintF("%02x ", (*data)[i]);
      } else {
        absl::PrintF("   ");
      }
    }
    absl::PrintF(" %s%s\n", nsasm::ToString(instruction->mnemonic),
                 nsasm::ArgsToString(instruction->addressing_mode,
                                     instruction->arg1, instruction->arg2));
    flag_state = flag_state.Execute(*instruction);
    pc = nsasm::AddToPC(pc, bytes_read);
  }
  return 0;
}