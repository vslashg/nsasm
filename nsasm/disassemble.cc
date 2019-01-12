#include "nsasm/disassemble.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "nsasm/decode.h"
#include "nsasm/error.h"

namespace nsasm {

namespace {

// Returns true if executing this instruction means control does not contine to
// the next.
bool IsExitInstruction(const Instruction& ins) {
  Mnemonic mnemonic = ins.mnemonic;
  return mnemonic == M_jmp || mnemonic == M_rtl || mnemonic == M_rts ||
         mnemonic == M_rti || mnemonic == M_stp || mnemonic == M_bra;
};

}  // namespace

ErrorOr<Disassembly> Disassemble(const Rom& rom, int starting_address,
                                 const FlagState& initial_flag_state) {
  // Mapping of instruction addresses to decoded instructions
  Disassembly result;

  // label generator
  int gensym_count = 0;
  auto gensym = [&gensym_count]() {
    return absl::StrCat("gensym", ++gensym_count);
  };

  // Mapping of instruction addresses to jump target label name
  std::map<int, std::string> label_names;
  auto get_label = [&label_names, &gensym](int address) {
    auto it = label_names.find(address);
    if (it == label_names.end()) {
      return label_names[address] = gensym();
    } else {
      return it->second;
    }
  };

  // The flag_state when execution reaches a given address
  std::map<int, FlagState> entry_state;

  // Map of locations to consider next, and the flag state to use
  // when considering it.
  std::map<int, FlagState> decode_stack;
  auto add_to_decode_stack = [&decode_stack](int address,
                                             const FlagState& state) {
    auto it = decode_stack.find(address);
    if (it == decode_stack.end()) {
      decode_stack[address] = state;
    } else {
      decode_stack[address] |= state;
    }
  };
  decode_stack[starting_address] = initial_flag_state;

  while (!decode_stack.empty()) {
    // service the lowest instruction we haven't considered
    auto next = *decode_stack.begin();
    decode_stack.erase(decode_stack.begin());

    int pc = next.first;
    const FlagState& current_flag_state = next.second;

    if (result.count(pc) == 0) {
      // This is the first time we've seen this address.  Try to disassemble
      // it.
      auto instruction_data = rom.Read(pc, 4);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(instruction_data, rom.path(), pc);
      auto instruction = Decode(*instruction_data, current_flag_state);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(instruction, rom.path(), pc);

      int instruction_bytes = InstructionLength(instruction->addressing_mode);

      int next_pc = AddToPC(pc, instruction_bytes);
      const FlagState new_flag_state = current_flag_state.Execute(*instruction);

      // If this instruction is relatively addressed, we need a label, and
      // need to add that address to code we should try to disassemble.
      if (instruction->addressing_mode == A_rel8 ||
          instruction->addressing_mode == A_rel16) {
        int value = instruction->arg1.ToValue();
        int target = AddToPC(next_pc, value);
        instruction->arg1 = Argument(value, get_label(target));
        add_to_decode_stack(target, new_flag_state);
      }

      // We've decoded an instruction!  Store it.
      DisassembledInstruction di;
      di.instruction = *instruction;
      result[pc] = std::move(di);

      // If this instruction doesn't terminate the subroutine, we need to
      // execute the next line as well.
      if (!IsExitInstruction(*instruction)) {
        add_to_decode_stack(next_pc, new_flag_state);
      }
    } else {
      // If we were ambitious, we could back-propogate new flag state info here.
    }
  }

  // Currently all labels are "gensym#"; change them to "label#" in order of appearance.
  int i = 0;
  absl::flat_hash_map<std::string, std::string> label_rewrite;
  for (auto& node : label_names) {
    std::string new_label = absl::StrCat("label", ++i);
    label_rewrite[node.second] = new_label;
    node.second = new_label;
  }
  for (auto& node : result) {
    Argument& arg1 = node.second.instruction.arg1;
    if (!arg1.Label().empty()) {
      arg1 = Argument(arg1.ToValue(), label_rewrite[arg1.Label()]);
    }
  }

  // Now add labels to all our disassembled instructions.
  for (const auto& label_name : label_names) {
    result[label_name.first].label = label_name.second;
  }

  // Pseudo-op folding.  Merge CLC/ADC and CLC/SBC into ADD and SUB, respectively.
  auto iter = result.begin();
  while (iter != result.end()) {
    auto next_iter = std::next(iter);
    if (next_iter == result.end()) {
      break;
    }
    if (iter->second.instruction.mnemonic == M_clc &&
        next_iter->second.instruction.mnemonic == M_adc &&
        next_iter->second.label.empty()) {
      iter->second.instruction = next_iter->second.instruction;
      iter->second.instruction.mnemonic = PM_add;
      result.erase(next_iter);
      continue;
    }
    if (iter->second.instruction.mnemonic == M_clc &&
        next_iter->second.instruction.mnemonic == M_sbc &&
        next_iter->second.label.empty()) {
      iter->second.instruction = next_iter->second.instruction;
      iter->second.instruction.mnemonic = PM_sub;
      result.erase(next_iter);
      continue;
    }
    iter = next_iter;
  }

  return result;
}

}  // namespace nsasm
