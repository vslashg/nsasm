#include "nsasm/listing.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string FormatDisassembly(const DisassemblyMap& disassembly) {
  std::string result;
  if (disassembly.empty()) {
    absl::StrAppendFormat(&result, "; Disassembled no instructions.\n");
    return result;
  }
  nsasm::Address pc = disassembly.begin()->first;
  absl::StrAppendFormat(&result, "; Disassembled %d instructions.\n",
                        disassembly.size());
  absl::StrAppendFormat(&result, "         .org %s\n", pc.ToString());
  for (const auto& value : disassembly) {
    if (value.first != pc) {
      absl::StrAppendFormat(&result, "         .org %s\n",
                            value.first.ToString());
      pc = value.first;
    }
    std::string label = value.second.label;
    const nsasm::Instruction& instruction = value.second.instruction;

    if (!label.empty()) {
      label += ':';
    }
    if (value.second.is_entry) {
      absl::StrAppendFormat(
          &result, "%-8s .entry %s\n", label,
          value.second.current_execution_state.Flags().ToString());
      label.clear();
    }
    std::string text = absl::StrFormat("%-8s %s", label, instruction.ToString());
    absl::StrAppendFormat(&result, "%-35s ; %s %s\n", text, pc.ToString(),
                          value.second.next_execution_state.Flags().ToString());

    pc = pc.AddWrapped(value.second.instruction.SerializedSize());
  }
  return result;
}

}  // namespace nsasm
