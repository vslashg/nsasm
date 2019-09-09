#ifndef NSASM_INSTRUCTION_H_
#define NSASM_INSTRUCTION_H_

#include "nsasm/addressing_mode.h"
#include "nsasm/calling_convention.h"
#include "nsasm/execution_state.h"
#include "nsasm/expression.h"
#include "nsasm/mnemonic.h"
#include "nsasm/output_sink.h"

namespace nsasm {

struct Instruction {
  Mnemonic mnemonic;
  AddressingMode addressing_mode;
  ExpressionOrNull arg1;
  ExpressionOrNull arg2;
  ReturnConvention return_convention;
  Location location;

  // Returns an error if this instruction's mnemonic and addressing mode are
  // inconsistent with the provided flag state.
  ErrorOr<void> CheckConsistency(const StatusFlags& status_flags) const;

  // If this instruction has a conditional addressing mode, change it to be
  // definite, based on the provided flag state.
  ErrorOr<void> FixAddressingMode(const StatusFlags& status_flags);

  // Returns true if executing this instruction means control does not contine
  // to the next.
  bool IsExitInstruction() const;

  // Returns true if this is a relative branch instruction.
  bool IsLocalBranch() const;

  // If this is a far branch, and the address can be determined, return the
  // target.  Else return nullopt.
  //
  // `source_address` is the address where this instruction resides in memory.
  // This is used to calculate the target bank address for jumps encoded with
  // only two bytes.
  absl::optional<int> FarBranchTarget(int source_address) const;

  // Update the provided execution state to reflect this instruction being run.
  //
  // This may attempt to evaluate this instruction's arguments with the provided
  // lookup context, depending on the instruction and addressing mode/.  If such
  // a lookup is attempted and fails, and if needs_reeval is provided, then
  // *needs_reeval is set true.
  ErrorOr<void> Execute(ExecutionState* execution_state,
                        const LookupContext& context = NullLookupContext(),
                        bool* needs_reeval = nullptr) const;

  // As above, but returns the state that results from a successful conditional
  // branch from this instruction.
  //
  // This difference matters for BCC/BCS.  For example, after BCC (branch if
  // carry clear), the C bit is set if we continue to the next instruction, and
  // clear if set.
  ErrorOr<void> ExecuteBranch(ExecutionState* execution_state) const;

  int SerializedSize() const;

  // Attempt to assemble this instruction to the given address and given sink.
  //
  // Returns an error if the instruction cannot be assembled for some reason.
  // Also forwards any error returned by the output sink.
  ErrorOr<void> Assemble(int address, const LookupContext& context,
                         OutputSink* sink) const;

  std::string ToString() const;
};

inline bool Instruction::IsExitInstruction() const {
  return mnemonic == M_jmp || mnemonic == M_rtl || mnemonic == M_rts ||
         mnemonic == M_rti || mnemonic == M_stp || mnemonic == M_bra ||
         mnemonic == M_brl || return_convention.IsExitCall();
};

inline bool Instruction::IsLocalBranch() const {
  return (addressing_mode == A_rel8 || addressing_mode == A_rel16) &&
         mnemonic != M_per;
};


}  // namespace nsasm

#endif  // NSASM_INSTRUCTION_H_
