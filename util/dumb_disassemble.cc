#include <map>

#include "absl/strings/str_format.h"
#include "nsasm/decode.h"
#include "nsasm/execution_state.h"
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

  auto parsed_flag = nsasm::StatusFlags::FromName(argv[3]);
  if (!parsed_flag) {
    absl::PrintF("%s does not name a processor mode\n", argv[3]);
    return 1;
  }

  int address = hex_address;
  nsasm::ExecutionState execution_state(*parsed_flag);
  std::map<int, nsasm::ExecutionState> local_jumps;
  while (true) {
    auto instruction_data = rom->Read(address, 4);
    if (!instruction_data.ok()) {
      absl::PrintF("%06x - ERROR: %s\n", address,
                   instruction_data.error().ToString());
      return 1;
    }
    auto instruction =
        nsasm::Decode(*instruction_data, execution_state.Flags());
    if (!instruction.ok()) {
      absl::PrintF("%06x - ERROR: %s\n", address,
                   instruction.error().ToString());
      return 1;
    }
    int instruction_bytes = InstructionLength(instruction->addressing_mode);
    int next_pc = nsasm::AddToPC(address, instruction_bytes);
    auto status = instruction->Execute(&execution_state);
    if (!status.ok()) {
      absl::PrintF("%06x - ERROR: %s\n", address, status.error().ToString());
      return 1;
    }
    std::string local_branch_target;
    if (instruction->IsLocalBranch()) {
      int target =
          next_pc + *instruction->arg1.Evaluate(nsasm::NullLookupContext());
      auto prev_value = local_jumps.find(target);
      local_branch_target = absl::StrFormat(" to $%06x", target);
      if (prev_value == local_jumps.end()) {
        local_branch_target += " (new)";
      } else {
        local_branch_target +=
            absl::StrFormat(" (was %s)", prev_value->second.Flags().ToString());
      }
      local_jumps[target] = execution_state;
    }
    std::string instruction_string = instruction->ToString();
    absl::PrintF("%06x - %30s ; %s%s\n", address, instruction_string,
                 execution_state.Flags().ToString(), local_branch_target);
    if (instruction->IsExitInstruction()) {
      auto next_instruction_it = local_jumps.lower_bound(next_pc);
      if (next_instruction_it == local_jumps.end()) {
        absl::PrintF("End of subroutine.\n");
        break;
      }
      if (next_instruction_it->first > next_pc) {
        absl::PrintF(
            "Gap found here.  Nearest local jump target is %06x (%s).\n",
            next_instruction_it->first,
            next_instruction_it->second.Flags().ToString());
        break;
      }
      execution_state = next_instruction_it->second;
    }
    address = next_pc;
  }
  if (!local_jumps.empty()) {
    absl::PrintF("Earliest branch target %06x (%s).\n",
                 local_jumps.begin()->first,
                 local_jumps.begin()->second.Flags().ToString());
  }
  return 0;
}
