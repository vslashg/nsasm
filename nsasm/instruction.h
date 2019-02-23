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

  // Returns true if this instruction's mnemonic and addressing mode are
  // consistent with the provided flag state.
  bool IsConsistent(const FlagState& flag_state);

  // Returns the new state that results from executing the given instruction
  // from the current state.
  ABSL_MUST_USE_RESULT FlagState Execute(FlagState flag_state) const;

  // As above, but returns the state that results from a successful conditional
  // branch from this instruction.
  //
  // This difference matters for BCC/BCS.  For example, after BCC (branch if
  // carry clear), the C bit is set if we continue to the next instruction, and
  // clear if set.
  ABSL_MUST_USE_RESULT FlagState ExecuteBranch(FlagState flag_state) const;

  std::string ToString() const;
};

}  // namespace nsasm

#endif  // NSASM_INSTRUCTION_H_
