#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "nsasm/decode.h"
#include "nsasm/instruction.h"

#include <cstdint>
#include <cstdio>

// Test utility to exercise

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom> <file-offset>\n\n"
      "Disassembles some code starting at the named offset.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    usage(argv[0]);
    return 0;
  }

  FILE* f = fopen(argv[1], "rb");
  if (!f) {
    absl::PrintF("Failure opening file.\n");
    return 1;
  }

  int offset;
  if (!absl::SimpleAtoi(argv[2], &offset)) {
    usage(argv[0]);
    return 1;
  }

  if (fseek(f, offset, SEEK_SET) == -1) {
    absl::PrintF("Unable to seek.\n");
    return 1;
  }

  uint8_t buffer[4096];
  size_t bytes_read = fread(buffer, 1, 4096, f);
  absl::Span<uint8_t> data(buffer, bytes_read);
  absl::PrintF("%x %x %x\n", data[0], data[1], data[2]);

  // Assume we are in native mode, with one-byte registers.
  //
  // This is a wild guess but works for my ad-hoc testing purposes at the
  // moment.
  nsasm::FlagState flag_state(nsasm::B_off, nsasm::B_on, nsasm::B_on);
  while (true) {
    auto instruction = nsasm::Decode(data, flag_state);
    if (!instruction.has_value()) {
      absl::PrintF("failed to decode...\n");
      return 0;
    }
    // Print a hexdecimal representation of the bytes read
    int bytes_read = InstructionLength(instruction->addressing_mode);
    for (int i = 0; i < 4; ++i) {
      if (i < bytes_read) {
        absl::PrintF("%02x ", data[i]);
      } else {
        absl::PrintF("   ");
      }
    }
    absl::PrintF(" %s%s\n", nsasm::ToString(instruction->mnemonic),
                 nsasm::ArgsToString(instruction->addressing_mode,
                                     instruction->arg1, instruction->arg2));
    flag_state = flag_state.Execute(*instruction);
    data.remove_prefix(InstructionLength(instruction->addressing_mode));
  }
  return 0;
}