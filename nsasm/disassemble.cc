#include "nsasm/disassemble.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "nsasm/decode.h"
#include "nsasm/error.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

ErrorOr<void> Disassembler::Disassemble(int starting_address,
                                       const FlagState& initial_flag_state) {
  // Mapping of instruction addresses to jump target label name
  std::map<int, std::string> label_names;
  auto get_label = [this, &label_names](int address) {
    // Is address in label map?
    auto it = label_names.find(address);
    if (it != label_names.end()) {
      return it->second;
    }
    // Was address given a label in a previous disassembly?
    auto it2 = disassembly_.find(address);
    if (it2 != disassembly_.end() && !it2->second.label.empty()) {
      return it2->second.label;
    }
    // Never before seen; generate a name now.
    return label_names[address] = GenSym();
  };

  // Map of locations to consider next, and the flag state to use
  // when considering it.
  std::map<int, FlagState> decode_stack;

  auto add_to_decode_stack = [&decode_stack](int address,
                                             const FlagState& state) {
    auto it = decode_stack.find(address);
    if (it == decode_stack.end()) {
      decode_stack[address] = state;
    } else {
      it->second |= state;
    }
  };

  add_to_decode_stack(starting_address, initial_flag_state);
  // ensure entry point is marked, and has a label
  entry_points_.insert(starting_address);
  get_label(starting_address);

  while (!decode_stack.empty()) {
    // service the lowest instruction we haven't considered
    auto next = *decode_stack.begin();
    decode_stack.erase(decode_stack.begin());

    int pc = next.first;
    const FlagState& current_flag_state = next.second;

    auto existing_instruction_iter = disassembly_.find(pc);
    if (existing_instruction_iter == disassembly_.end()) {
      // This is the first time we've seen this address.  Try to disassemble
      // it.
      auto instruction_data = rom_.Read(pc, 4);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(instruction_data, rom_.path(), pc);
      auto instruction = Decode(*instruction_data, current_flag_state);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(instruction, rom_.path(), pc);

      int instruction_bytes = InstructionLength(instruction->addressing_mode);

      int next_pc = AddToPC(pc, instruction_bytes);
      auto next_flag_state = instruction->Execute(current_flag_state);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(next_flag_state, rom_.path(), pc);

      // If this instruction is relatively addressed, we need a label, and
      // need to add that address to code we should try to disassemble.
      if (instruction->IsLocalBranch()) {
        int value = *instruction->arg1.Evaluate(NullLookupContext());
        int target = AddToPC(next_pc, value);
        instruction->arg1.ApplyLabel(get_label(target));
        auto branch_flag_state = instruction->ExecuteBranch(current_flag_state);
        NSASM_RETURN_IF_ERROR_WITH_LOCATION(branch_flag_state, rom_.path(), pc);
        add_to_decode_stack(target, *branch_flag_state);
      }

      // We've decoded an instruction!  Store it.
      DisassembledInstruction di;
      di.instruction = std::move(*instruction);
      di.current_flag_state = current_flag_state;
      di.next_flag_state = *next_flag_state;
      disassembly_[pc] = std::move(di);

      // If this instruction doesn't terminate the subroutine, we need to
      // execute the next line as well.
      if (!di.instruction.IsExitInstruction()) {
        add_to_decode_stack(next_pc, *next_flag_state);
      }
    } else {
      // We've been here before.  Weaken the incoming state bits for this
      // instruction to allow for the new input flag state.  If this represents
      // a change, check that the resulting state is still consistent, and
      // propagate the changed flag state bits forward.
      DisassembledInstruction& di = existing_instruction_iter->second;
      FlagState combined_flag_state =
          current_flag_state | di.current_flag_state;
      if (combined_flag_state != di.current_flag_state) {
        di.current_flag_state = combined_flag_state;
        auto next_flag_state = di.instruction.Execute(combined_flag_state);
        NSASM_RETURN_IF_ERROR_WITH_LOCATION(next_flag_state, rom_.path(), pc);
        di.next_flag_state = *next_flag_state;
        int instruction_bytes =
            InstructionLength(di.instruction.addressing_mode);
        int next_pc = AddToPC(pc, instruction_bytes);
        add_to_decode_stack(next_pc, di.next_flag_state);
        if (di.instruction.addressing_mode == A_rel8 ||
            di.instruction.addressing_mode == A_rel16) {
          int value = *di.instruction.arg1.Evaluate(NullLookupContext());
          int target = AddToPC(next_pc, value);
          auto branch_flag_state =
              di.instruction.ExecuteBranch(combined_flag_state);
          NSASM_RETURN_IF_ERROR_WITH_LOCATION(branch_flag_state, rom_.path(),
                                              pc);
          add_to_decode_stack(target, *branch_flag_state);
        }
      }
    }
  }

  // Apply labels to all instructions.
  for (const auto& label_name : label_names) {
    disassembly_[label_name.first].label = label_name.second;
  }

  return {};
}

ErrorOr<void> Disassembler::Cleanup() {
  // Currently all labels are "gensym#" in visitation order; change them to
  // "label#" or "entry#", in address order.
  int next_label = 0;
  int next_entry = 0;
  absl::flat_hash_map<std::string, std::string> label_rewrite;
  for (auto& node : disassembly_) {
    if (!node.second.label.empty()) {
      bool is_entry_point = (entry_points_.count(node.first) > 0);
      std::string new_label = is_entry_point
                                  ? absl::StrCat("entry", ++next_entry)
                                  : absl::StrCat("label", ++next_label);
      label_rewrite[node.second.label] = new_label;
      node.second.label = new_label;
      node.second.is_entry = is_entry_point;
    }
  }

  for (auto& node : disassembly_) {
    ExpressionOrNull& arg1 = node.second.instruction.arg1;
    if (arg1.IsLabel()) {
      arg1.ApplyLabel(label_rewrite[arg1.ToString()]);
    }
  }

  // Now add labels to all our disassembled instructions.

  // Pseudo-op folding.  Merge CLC/ADC and CLC/SBC into ADD and SUB,
  // respectively.
  auto iter = disassembly_.begin();
  while (iter != disassembly_.end()) {
    auto next_iter = std::next(iter);
    if (next_iter == disassembly_.end()) {
      break;
    }
    if (iter->second.instruction.mnemonic == M_clc &&
        next_iter->second.instruction.mnemonic == M_adc &&
        next_iter->second.label.empty()) {
      iter->second.instruction = std::move(next_iter->second.instruction);
      iter->second.instruction.mnemonic = PM_add;
      iter->second.next_flag_state = next_iter->second.next_flag_state;
      disassembly_.erase(next_iter);
      continue;
    }
    if (iter->second.instruction.mnemonic == M_sec &&
        next_iter->second.instruction.mnemonic == M_sbc &&
        next_iter->second.label.empty()) {
      iter->second.instruction = std::move(next_iter->second.instruction);
      iter->second.instruction.mnemonic = PM_sub;
      iter->second.next_flag_state = next_iter->second.next_flag_state;
      disassembly_.erase(next_iter);
      continue;
    }
    iter = next_iter;
  }

  return {};
}

}  // namespace nsasm
