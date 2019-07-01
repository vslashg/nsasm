#include "absl/strings/str_format.h"
#include "nsasm/decode.h"
#include "nsasm/flag_state.h"
#include "nsasm/rom.h"

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom> <address> <mode name>\n\n"
      "Dumbly disassemble instructions starting at the given address, but "
      "does\n"
      "not follow branches or create labels.  Halts when either an error\n"
      "occurs, or an exit instruction is reached.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    usage(argv[0]);
    return 0;
  }

  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("%s\n", rom.error().ToString());
    return 1;
  }

  unsigned hex_address;
  if (!sscanf(argv[2], "%x", &hex_address)) {
    usage(argv[2]);
    return 1;
  }

  auto parsed_flag = nsasm::FlagState::FromName(argv[3]);
  if (!parsed_flag) {
    absl::PrintF("%s does not name a processor mode\n", argv[3]);
    return 1;
  }

  int address = hex_address;
  nsasm::FlagState flag_state = *parsed_flag;
  while (true) {
    auto instruction_data = rom->Read(address, 4);
    if (!instruction_data.ok()) {
      absl::PrintF("%06x - ERROR: %s\n", address,
                   instruction_data.error().ToString());
      return 1;
    }
    auto instruction = nsasm::Decode(*instruction_data, flag_state);
    if (!instruction.ok()) {
      absl::PrintF("%06x - ERROR: %s\n", address,
                   instruction.error().ToString());
      return 1;
    }
    int instruction_bytes = InstructionLength(instruction->addressing_mode);
    int next_pc = nsasm::AddToPC(address, instruction_bytes);
    auto next_flag_state = instruction->Execute(flag_state);
    if (!next_flag_state.ok()) {
      absl::PrintF("%06x - ERROR: %s\n", address,
                   next_flag_state.error().ToString());
      return 1;
    }
    flag_state = *next_flag_state;
    std::string local_branch_target;
    if (instruction->IsLocalBranch()) {
      int target =
          next_pc + *instruction->arg1.Evaluate(nsasm::NullLookupContext());
      local_branch_target = absl::StrFormat(" to $%06x", target);
    }
    std::string instruction_string = instruction->ToString();
    absl::PrintF("%06x - %30s ; %s%s\n", address, instruction_string,
                 flag_state.ToString(), local_branch_target);
    if (instruction->IsExitInstruction()) {
      break;
    }
    address = next_pc;
  }
  return 0;
}
