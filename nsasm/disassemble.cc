#include "nsasm/disassemble.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "nsasm/decode.h"
#include "nsasm/error.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

ErrorOr<std::map<nsasm::Address, StatusFlags>> Disassembler::Disassemble(
    nsasm::Address starting_address, const StatusFlags& initial_status_flags) {
  // Map of newly decoded, or locally modified, instructions.  This is
  // written to disassembly_ at the end of this function, assuming we did not
  // exit with an error.
  std::map<nsasm::Address, DisassembledInstruction> new_disassembly;
  auto get_instruction =
      [this,
       &new_disassembly](nsasm::Address address) -> DisassembledInstruction* {
    // return the instruction if it already is in the new map
    auto new_disassembly_it = new_disassembly.find(address);
    if (new_disassembly_it != new_disassembly.end()) {
      return &new_disassembly_it->second;
    };
    // otherwise, if it's in this class's state, copy it over into the new map
    // and return the copy
    auto old_disassembly_it = disassembly_.find(address);
    if (old_disassembly_it != disassembly_.end()) {
      DisassembledInstruction& copy = new_disassembly[address];
      copy = old_disassembly_it->second;
      return &copy;
    }
    return nullptr;
  };

  // Mapping of instruction addresses to jump target label name
  std::map<nsasm::Address, std::string> label_names;
  auto get_label = [this, &label_names](nsasm::Address address) {
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

  // Map of locations to consider next, and the execution state to use
  // when considering it.
  std::map<nsasm::Address, ExecutionState> decode_stack;
  auto add_to_decode_stack = [&decode_stack](nsasm::Address address,
                                             const ExecutionState& state) {
    auto it = decode_stack.find(address);
    if (it == decode_stack.end()) {
      decode_stack[address] = state;
    } else {
      it->second |= state;
    }
  };

  // Map of far branch targets to incoming states
  std::map<nsasm::Address, StatusFlags> far_branch_targets;
  auto add_far_branch = [&far_branch_targets](nsasm::Address address,
                                              const ExecutionState& state) {
    auto it = far_branch_targets.find(address);
    if (it == far_branch_targets.end()) {
      far_branch_targets[address] = state.Flags();
    } else {
      it->second |= state.Flags();
    }
  };

  ExecutionState initial_execution_state(initial_status_flags);

  add_to_decode_stack(starting_address, initial_execution_state);
  // ensure entry point is marked, and has a label
  entry_points_.insert(starting_address);
  get_label(starting_address);

  while (!decode_stack.empty()) {
    // service the lowest instruction we haven't considered
    auto next = *decode_stack.begin();
    decode_stack.erase(decode_stack.begin());

    nsasm::Address pc = next.first;
    const ExecutionState& current_execution_state = next.second;

    DisassembledInstruction* existing_instruction = get_instruction(pc);
    if (!existing_instruction) {
      // This is the first time we've seen this address.  Try to disassemble
      // it.
      auto instruction_data = src_->Read(pc, 4);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(instruction_data, src_->Path(), pc);
      auto instruction =
          Decode(*instruction_data, current_execution_state.Flags());
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(instruction, src_->Path(), pc);

      int instruction_bytes = InstructionLength(instruction->addressing_mode);

      nsasm::Address next_pc = pc.AddWrapped(instruction_bytes);
      auto next_execution_state = current_execution_state;
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(
          instruction->Execute(&next_execution_state), src_->Path(), pc);

      auto far_branch_address = instruction->FarBranchTarget(pc);
      if (far_branch_address.has_value()) {
        nsasm::Address target = *far_branch_address;
        add_far_branch(target, next_execution_state);
        if (instruction->mnemonic == M_jsr || instruction->mnemonic == M_jsl) {
          // If the subroutine call requires a yield, add that to disassembly.
          auto return_conventions_it = return_conventions_.find(target);
          if (return_conventions_it != return_conventions_.end()) {
            instruction->return_convention = return_conventions_it->second;
          }
        }
        instruction->return_convention.ApplyTo(&next_execution_state);
      }

      // If this instruction is relatively addressed, we need a label, and
      // need to add that address to code we should try to disassemble.
      if (instruction->IsLocalBranch()) {
        int value = *instruction->arg1.Evaluate(NullLookupContext());
        nsasm::Address target = next_pc.AddWrapped(value);
        instruction->arg1.ApplyLabel(get_label(target));
        auto branch_execution_state = current_execution_state;
        NSASM_RETURN_IF_ERROR_WITH_LOCATION(
            instruction->ExecuteBranch(&branch_execution_state), src_->Path(),
            pc);
        add_to_decode_stack(target, branch_execution_state);
      }

      // We've decoded an instruction!  Store it.
      DisassembledInstruction di;
      di.instruction = std::move(*instruction);
      di.current_execution_state = current_execution_state;
      di.next_execution_state = next_execution_state;
      new_disassembly[pc] = std::move(di);

      // If this instruction doesn't terminate the subroutine, we need to
      // execute the next line as well.
      if (!di.instruction.IsExitInstruction()) {
        add_to_decode_stack(next_pc, next_execution_state);
      }
    } else {
      // We've been here before.  Weaken the incoming state bits for this
      // instruction to allow for the new input flag state.  If this represents
      // a change, check that the resulting state is still consistent, and
      // propagate the changed flag state bits forward.
      DisassembledInstruction& di = *existing_instruction;
      ExecutionState combined_execution_state =
          current_execution_state | di.current_execution_state;
      if (combined_execution_state != di.current_execution_state) {
        // Check that the instruction still decodes with the new flag state.
        // (We can throw the answer away if so, since we've already disassembled
        // this instruction before.)
        auto instruction_data = src_->Read(pc, 4);
        NSASM_RETURN_IF_ERROR_WITH_LOCATION(
            Decode(*instruction_data, combined_execution_state.Flags()),
            src_->Path(), pc);

        // Update the flag state on this instruction
        di.current_execution_state = combined_execution_state;
        auto next_execution_state = combined_execution_state;

        NSASM_RETURN_IF_ERROR_WITH_LOCATION(
            di.instruction.Execute(&next_execution_state), src_->Path(), pc);
        di.next_execution_state = next_execution_state;
        int instruction_bytes =
            InstructionLength(di.instruction.addressing_mode);
        nsasm::Address next_pc = pc.AddWrapped(instruction_bytes);

        // Propagate the changed state forward to the next instruction...
        if (!di.instruction.IsExitInstruction()) {
          add_to_decode_stack(next_pc, next_execution_state);
        }
        // ... the far branch target ...
        auto far_branch_address = di.instruction.FarBranchTarget(pc);
        if (far_branch_address.has_value()) {
          add_far_branch(*far_branch_address, next_execution_state);
        }
        // ... and the local branch target.
        if (di.instruction.IsLocalBranch()) {
          auto branch_execution_state = current_execution_state;
          NSASM_RETURN_IF_ERROR_WITH_LOCATION(
              di.instruction.ExecuteBranch(&branch_execution_state),
              src_->Path(), pc);
          int value = *di.instruction.arg1.Evaluate(NullLookupContext());
          nsasm::Address target = next_pc.AddWrapped(value);
          add_to_decode_stack(target, branch_execution_state);
        }
      }
    }
  }

  // If we got here, disassembly was a success.

  // Add a suffix to each new disassembly instruction that supports one.
  for (auto& node : new_disassembly) {
    Instruction& ins = node.second.instruction;
    const StatusFlags& flags = node.second.current_execution_state.Flags();
    StatusFlagUsed flag_used = FlagControllingInstructionSize(ins.mnemonic);
    if (flag_used == kUsesMFlag) {
      if (flags.MBit() == B_on) {
        ins.suffix = S_b;
      } else if (flags.MBit() == B_off) {
        ins.suffix = S_w;
      }
    }
    if (flag_used == kUsesXFlag) {
      if (flags.XBit() == B_on) {
        ins.suffix = S_b;
      } else if (flags.XBit() == B_off) {
        ins.suffix = S_w;
      }
    }
  }

  // Copy the entries from the temporary map to the permanent state.
  for (const auto& node : new_disassembly) {
    disassembly_[node.first] = node.second;
  }

  // Apply labels to all instructions.
  for (const auto& label_name : label_names) {
    disassembly_[label_name.first].label = label_name.second;
  }

  return far_branch_targets;
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

  // Refer to far jump targets by name.
  for (auto& node : disassembly_) {
    Instruction& instruction = node.second.instruction;
    auto jump_target = instruction.FarBranchTarget(node.first);
    if (jump_target) {
      auto target_name = NameForAddress(*jump_target);
      if (target_name) {
        if (instruction.addressing_mode == A_dir_l) {
          instruction.arg1 = absl::make_unique<IdentifierExpression>(
              FullIdentifier(*target_name), T_long);
        } else if (instruction.addressing_mode == A_dir_w) {
          instruction.arg1 = absl::make_unique<IdentifierExpression>(
              FullIdentifier(*target_name), T_word);
        }
      }
    }
  }

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
      iter->second.next_execution_state =
          next_iter->second.next_execution_state;
      disassembly_.erase(next_iter);
      continue;
    }
    if (iter->second.instruction.mnemonic == M_sec &&
        next_iter->second.instruction.mnemonic == M_sbc &&
        next_iter->second.label.empty()) {
      iter->second.instruction = std::move(next_iter->second.instruction);
      iter->second.instruction.mnemonic = PM_sub;
      iter->second.next_execution_state =
          next_iter->second.next_execution_state;
      disassembly_.erase(next_iter);
      continue;
    }
    iter = next_iter;
  }

  return {};
}

absl::optional<std::string> Disassembler::NameForAddress(
    nsasm::Address address) {
  if (entry_points_.count(address) == 0) {
    return absl::nullopt;
  }
  auto it = disassembly_.find(address);
  if (it != disassembly_.end() && !it->second.label.empty()) {
    return it->second.label;
  }
  return absl::nullopt;
}

}  // namespace nsasm
