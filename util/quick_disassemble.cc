#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "nsasm/decode.h"
#include "nsasm/disassemble.h"
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
  if (!rom.ok()) {
    absl::PrintF("%s\n", rom.error().ToString());
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

  auto disassembly = nsasm::Disassemble(*rom, pc, flag_state);
  if (!disassembly.ok()) {
    absl::PrintF("%s\n", disassembly.error().ToString());
  } else {
    absl::PrintF("Disassembled %d instructions.\n", disassembly->size());
    for (const auto& value : *disassembly) {
      int pc = value.first;
      std::string label = value.second.label;
      const nsasm::Instruction& instruction = value.second.instruction;

      std::string text =
          absl::StrFormat("%06x %-8s %s", pc, label, instruction.ToString());
      absl::PrintF("%-30s ;%s\n", text, value.second.flag_state.ToString());
    }
  }
}