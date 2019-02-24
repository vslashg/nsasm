#include "nsasm/instruction.h"

#include "absl/strings/str_format.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

std::string Instruction::ToString() const {
  return absl::StrFormat("%s%s", nsasm::ToString(mnemonic),
                         ArgsToString(addressing_mode, arg1, arg2));
}

ErrorOr<void> Instruction::CheckConsistency(const FlagState& flag_state) const {
  Mnemonic effective_mnemonic = mnemonic;
  if (mnemonic == PM_add || mnemonic == PM_sub) {
    // ADD and SUB aren't real mnemonics, but follow the same addressing
    // rules as ADC.
    effective_mnemonic = M_adc;
  }

  if (!IsLegalCombination(effective_mnemonic, addressing_mode)) {
    return Error(
        "logic error: instruction %s with addressing mode %s is inconsistent",
        nsasm::ToString(mnemonic), nsasm::ToString(addressing_mode));
  }

  if (addressing_mode == A_imm_fm) {
    // only legal if we know the state of the `m` bit
    if (flag_state.MBit() != B_on && flag_state.MBit() != B_off) {
      return Error(
          "instruction %s with immediate argument depends on `m` flag state, "
          "which is unknown here",
          nsasm::ToString(mnemonic));
    }
  } else if (addressing_mode == A_imm_fx) {
    // as above, but for the `x` bit
    if (flag_state.XBit() != B_on && flag_state.XBit() != B_off) {
      return Error(
          "instruction %s with immediate argument depends on `x` flag state, "
          "which is unknown here",
          nsasm::ToString(mnemonic));
    }
  }
  return {};
}

ErrorOr<FlagState> Instruction::Execute(const FlagState& flag_state_in) const {
  NSASM_RETURN_IF_ERROR(CheckConsistency(flag_state_in));

  FlagState flag_state = flag_state_in;
  const Mnemonic& m = mnemonic;

  // Instructions that clear or set carry bit (used to prime the XCE
  // instruction, which swaps the carry bit and emulation bit.)

  // BCC and BCS essentially set and clear the c bit for the next instruction,
  // respectively, because if the bit is in the opposite state, we will branch
  // instead.
  if (m == M_sec || m == M_bcc) {
    flag_state.SetCBit(B_on);
    return flag_state;
  } else if (m == M_clc || m == M_bcs) {
    flag_state.SetCBit(B_off);
    return flag_state;
  }

  // Instructions that clear or set status bits explicitly
  if (m == M_rep || m == M_sep) {
    BitState target = (m == M_rep) ? B_off : B_on;
    auto arg = arg1.Evaluate();
    if (!arg.ok()) {
      // If REP or SEP are invoked with an unknown argument (a constant pulled
      // from another module, say), we will have to account for the ambiguity.
      //
      // Each bit will either be set to `target` or else left alone.  If the
      // current value of a bit is equal to `target`, it's unchanged; otherwise
      // it becomes ambiguous.
      if (flag_state.CBit() != target) {
        flag_state.SetCBit(B_unknown);
      }
      if (flag_state.XBit() != target) {
        flag_state.SetXBit(B_unknown);
      }
      if (flag_state.MBit() != target) {
        flag_state.SetMBit(B_unknown);
      }
      return flag_state;
    }

    // If the argument is known, we can set the effected bits.
    if (*arg & 0x01) {
      flag_state.SetCBit(target);
    }
    if (*arg & 0x10) {
      flag_state.SetXBit(target);
    }
    if (*arg & 0x20) {
      flag_state.SetMBit(target);
    }
    return flag_state;
  }

  // Instructions that push or pull the status bits onto the stack.
  // This heuristic doesn't attempt to track the stack pointer; we
  // just assume a PLP instruction gets the last value pushed by PHP.
  if (m == M_php) {
    flag_state.PushFlags();
    return flag_state;
  } else if (m == M_plp) {
    flag_state.PullFlags();
    return flag_state;
  }

  // Instruction that swaps the c and e bits.  This can change the
  // m and x bits as a side effect.
  if (m == M_xce) {
    flag_state.ExchangeCE();
    return flag_state;
  }

  // Instructions that use the c bit to indicate carry.  For the
  // purposes of this static analysis, treat these as setting the c
  // bit to an unknown state.
  if (m == M_adc || m == M_sbc || m == PM_add || m == PM_sub || m == M_cmp ||
      m == M_cpx || m == M_cpy || m == M_asl || m == M_lsr || m == M_rol ||
      m == M_ror) {
    flag_state.SetCBit(B_unknown);
    return flag_state;
  }

  // Subroutine and interrupt calls.  This logic will get more robust as we
  // introduce calling conventions, but for now we should assume these trash the
  // carry bit.
  if (m == M_jmp || m == M_jsl || m == M_jsr || m == M_brk || m == M_cop) {
    flag_state.SetCBit(B_unknown);
    return flag_state;
  }

  // Other instructions don't effect the flag state.
  return flag_state;
}

ErrorOr<FlagState> Instruction::ExecuteBranch(
    const FlagState& flag_state_in) const {
  auto flag_state = Execute(flag_state_in);
  NSASM_RETURN_IF_ERROR(flag_state);
  if (mnemonic == M_bcc) {
    flag_state->SetCBit(B_off);
  } else if (mnemonic == M_bcs) {
    flag_state->SetCBit(B_on);
  }
  return flag_state;
}

}  // namespace nsasm
