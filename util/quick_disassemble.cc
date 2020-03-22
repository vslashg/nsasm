#include <cstdint>
#include <cstdio>

#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "nsasm/decode.h"
#include "nsasm/disassemble.h"
#include "nsasm/instruction.h"
#include "nsasm/rom.h"

// Test utility to exercise disassembly

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom> ([@]<snes-hex-address> <mode name>)+\n\n"
      "Disassembles some code starting at the named offset.\n"
      "If the offset begins with @, dereference the 16-bit address at this "
      "location.\n",
      path);
}

void CombineStates(
    std::map<nsasm::Address, nsasm::StatusFlags>* targets,
    const std::map<nsasm::Address, nsasm::StatusFlags>& new_targets) {
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
  if (argc < 4) {
    usage(argv[0]);
    return 0;
  }

  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("%s\n", rom.error().ToString());
    return 1;
  }

  std::map<nsasm::Address, nsasm::StatusFlags> seeds;
  for (int idx = 2; idx < argc - 1; idx += 2) {
    auto parsed_flag = nsasm::StatusFlags::FromName(argv[idx + 1]);
    if (!parsed_flag) {
      absl::PrintF("%s does not name a processor mode\n", argv[idx + 1]);
      return 1;
    }
    unsigned rd_address;
    const char* address = argv[idx];
    bool indirect = (address[0] == '@');
    if (indirect) ++address;
    if (!sscanf(address, "%x", &rd_address)) {
      usage(argv[0]);
      return 1;
    }
    if (indirect) {
      auto indirect_address = rom->ReadWord(nsasm::Address(rd_address));
      if (!indirect_address.ok()) {
        absl::PrintF("%s\n", indirect_address.error().ToString());
        return 1;
      }
      rd_address = *indirect_address;
    }
    seeds.emplace(nsasm::Address(rd_address), *parsed_flag);
  }

  nsasm::Disassembler disassembler(*std::move(rom));

  for (int pass = 0; pass < 100; ++pass) {
    std::map<nsasm::Address, nsasm::StatusFlags> new_seeds;
    for (const auto& node : seeds) {
      auto branch_targets = disassembler.Disassemble(node.first, node.second);
      if (!branch_targets.ok()) {
        absl::PrintF("; ERROR branching to %s with mode %s\n",
                     node.first.ToString(), node.second.ToString());
        absl::PrintF(";   %s\n", branch_targets.error().ToString());
      } else {
        CombineStates(&new_seeds, *branch_targets);
      }
    }
    seeds = std::move(new_seeds);
  }
  auto status = disassembler.Cleanup();
  if (!status.ok()) {
    absl::PrintF("%s\n", status.error().ToString());
    return 1;
  }

  const nsasm::DisassemblyMap& disassembly = disassembler.Result();
  if (disassembly.empty()) {
    absl::PrintF("; Disassembled no instructions.\n");
  } else {
    nsasm::Address pc = disassembly.begin()->first;
    absl::PrintF("; Disassembled %d instructions.\n", disassembly.size());
    absl::PrintF("         .org %s\n", pc.ToString());
    for (const auto& value : disassembly) {
      if (value.first != pc) {
        absl::PrintF("         .org %s\n", value.first.ToString());
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
      absl::PrintF("%-35s ; %s %s\n", text, pc.ToString(),
                   value.second.next_execution_state.Flags().ToString());

      pc = pc.AddWrapped(value.second.instruction.SerializedSize());
    }
  }
}
