#include "absl/strings/str_format.h"
#include "nsasm/assembler.h"
#include "nsasm/disassemble.h"
#include "nsasm/rom.h"

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom-file> {<path-to-asm-file> ...}\n\n"
      "Assemble the provided .asm files, and validate that their output\n"
      "matches the contents of hte provided ROM.\n\n"
      "On success, start disassembling at all remote jump targets found\n"
      "in the provided .asm file.\n",
      path);
}

void CombineStates(std::map<int, nsasm::StatusFlags>* targets,
                   const std::map<int, nsasm::StatusFlags>& new_targets) {
  for (const auto& node : new_targets) {
    auto it = targets->find(node.first);
    if (it == targets->end()) {
      (*targets)[node.first] = node.second;
    } else {
      it->second |= node.second;
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    usage(argv[0]);
    return 0;
  }
  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("Error loading ROM: %s\n", rom.error().ToString());
    return 1;
  }
  nsasm::Assembler assembler;
  for (int arg_index = 2; arg_index < argc; ++arg_index) {
    auto result = assembler.AddAsmFile(argv[arg_index]);
    if (!result.ok()) {
      absl::PrintF("Error assembling: %s\n", result.error().ToString());
    }
  }

  nsasm::RomIdentityTest rom_identity(&*rom);
  auto status = assembler.Assemble(&rom_identity);
  if (!status.ok()) {
    absl::PrintF("Error in assembly: %s\n", status.error().ToString());
    return 0;
  }

  std::map<int, nsasm::StatusFlags> seeds = assembler.JumpTargets();
  std::map<int, nsasm::ReturnConvention> return_conventions =
      assembler.JumpTargetReturnConventions();

  nsasm::Disassembler disassembler(*std::move(rom));
  disassembler.AddTargetReturnConventions(return_conventions);

  for (int pass = 0; pass < 100; ++pass) {
    std::map<int, nsasm::StatusFlags> new_seeds;
    for (const auto& node : seeds) {
      if (assembler.Contains(node.first)) {
        // This function is already disassembled in our input.
        continue;
      }
      auto branch_targets = disassembler.Disassemble(node.first, node.second);
      if (!branch_targets.ok()) {
        absl::PrintF("; ERROR branching to $%06x with mode %s\n",
          node.first, node.second.ToString());
        absl::PrintF(";   %s\n", branch_targets.error().ToString());
      } else {
        CombineStates(&new_seeds, *branch_targets);
      }
    }
    seeds = std::move(new_seeds);
  }
  status = disassembler.Cleanup();
  if (!status.ok()) {
    absl::PrintF("%s\n", status.error().ToString());
    return 1;
  }

  const nsasm::DisassemblyMap& disassembly = disassembler.Result();
  if (disassembly.empty()) {
    absl::PrintF("; Disassembled no instructions.\n");
  } else {
    int pc = disassembly.begin()->first;
    absl::PrintF("; Disassembled %d instructions.\n", disassembly.size());
    absl::PrintF("         .org $%06x\n", pc);
    for (const auto& value : disassembly) {
      if (value.first != pc) {
        absl::PrintF("         .org $%06x\n", value.first);
        pc = value.first;
      }
      std::string label = value.second.label;
      const nsasm::Instruction& instruction = value.second.instruction;

      if (!label.empty()) {
        label += ':';
      }
      if (value.second.is_entry) {
        absl::PrintF("%-8s .entry %s\n", label,
                     value.second.current_execution_state.Flags().ToString());
        label.clear();
      }
      std::string text =
          absl::StrFormat("%-8s %s", label, instruction.ToString());
      absl::PrintF("%-35s ; %06x %s\n", text, pc,
                   value.second.next_execution_state.Flags().ToString());

      pc += value.second.instruction.SerializedSize();
    }
  }
}