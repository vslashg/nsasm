#ifndef NSASM_INSTRUCTION_H_
#define NSASM_INSTRUCTION_H_

#include "nsasm/addressing_mode.h"
#include "nsasm/expression.h"
#include "nsasm/flag_state.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

struct Instruction {
  Mnemonic mnemonic;
  AddressingMode addressing_mode;
  ExpressionOrNull arg1;
  ExpressionOrNull arg2;
  Location location;

  // Returns an error if this instruction's mnemonic and addressing mode are
  // inconsistent with the provided flag state.
  ErrorOr<void> CheckConsistency(const FlagState& flag_state) const;

  // If this instruction has a conditional addressing mode, change it to be
  // definite, based on the provided flag state.
  void FixAddressingMode(const FlagState& flag_state);

  // Returns true if executing this instruction means control does not contine
  // to the next.
  bool IsExitInstruction() const;

  // Returns true if this is a relative branch instruction.
  bool IsLocalBranch() const;

  // Returns the new state that results from executing the given instruction
  // from the current state.
  ABSL_MUST_USE_RESULT ErrorOr<FlagState> Execute(
      const FlagState& flag_state) const;

  // As above, but returns the state that results from a successful conditional
  // branch from this instruction.
  //
  // This difference matters for BCC/BCS.  For example, after BCC (branch if
  // carry clear), the C bit is set if we continue to the next instruction, and
  // clear if set.
  ABSL_MUST_USE_RESULT ErrorOr<FlagState> ExecuteBranch(
      const FlagState& flag_state) const;

  std::string ToString() const;
};

inline bool Instruction::IsExitInstruction() const {
  return mnemonic == M_jmp || mnemonic == M_rtl || mnemonic == M_rts ||
         mnemonic == M_rti || mnemonic == M_stp || mnemonic == M_bra;
};

inline bool Instruction::IsLocalBranch() const {
  return (addressing_mode == A_rel8 || addressing_mode == A_rel16) &&
         mnemonic != M_per;
};


}  // namespace nsasm

#endif  // NSASM_INSTRUCTION_H_
