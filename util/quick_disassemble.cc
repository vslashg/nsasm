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
      "Usage: %s <path-to-rom> [@]<snes-hex-address> [<mode name>]\n\n"
      "Disassembles some code starting at the named offset.\n"
      "If the offset begins with @, dereference the 16-bit address at this "
      "location.\n",
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

  // Default to native mode, with one-byte A and X/Y.
  nsasm::FlagState flag_state(nsasm::B_off, nsasm::B_on, nsasm::B_on);
  if (argc > 3) {
    auto parsed = nsasm::FlagState::FromName(argv[3]);
    if (!parsed) {
      absl::PrintF("%s does not name a processor mode\n", argv[3]);
      return 1;
    }
    flag_state = *parsed;
  }

  unsigned rd_address;
  const char* address = argv[2];
  bool indirect = (address[0] == '@');
  if (indirect) ++address;
  if (!sscanf(address, "%x", &rd_address)) {
    usage(argv[0]);
    return 1;
  }
  if (indirect) {
    auto indirect_address = rom->ReadWord(rd_address);
    if (!indirect_address.ok()) {
      absl::PrintF("%s\n", indirect_address.error().ToString());
      return 1;
    }
    rd_address = *indirect_address;
  }
  int pc = rd_address;

  auto disassembly = nsasm::Disassemble(*rom, pc, flag_state);
  if (!disassembly.ok()) {
    absl::PrintF("%s\n", disassembly.error().ToString());
  } else {
    absl::PrintF("Disassembled %d instructions.\n", disassembly->size());
    absl::PrintF("%06x          .org $%06x\n", rd_address, rd_address);
    for (const auto& value : *disassembly) {
      int pc = value.first;
      std::string label = value.second.label;
      const nsasm::Instruction& instruction = value.second.instruction;

      std::string text =
          absl::StrFormat("%06x %-8s %s", pc, label, instruction.ToString());
      absl::PrintF("%-30s ;%s\n", text, value.second.next_flag_state.ToString());
    }
  }
}
