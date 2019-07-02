#include "nsasm/instruction.h"

#include "absl/strings/str_format.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

std::string Instruction::ToString() const {
  return absl::StrFormat("%s%s%s", nsasm::ToString(mnemonic),
                         ArgsToString(addressing_mode, arg1, arg2),
                         return_convention.ToSuffixString());
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
  } else if (addressing_mode == A_imm_b || addressing_mode == A_imm_w) {
    BitState needed_bit = (addressing_mode == A_imm_b) ? B_on : B_off;
    BitState actual_bit;
    char target_flag;
    if (ImmediateArgumentUsesMBit(mnemonic)) {
      target_flag = 'm';
      actual_bit = flag_state.MBit();
    } else if (ImmediateArgumentUsesXBit(mnemonic)) {
      target_flag = 'x';
      actual_bit = flag_state.XBit();
    } else {
      // This instruction doesn't depend on the `m` or `x` flag states, so
      // there's no flag state to check consistency against.
      return {};
    }

    if (actual_bit == B_unknown || actual_bit == B_original) {
      return Error(
          "instruction %s with immediate argument depends on `%c` flag state, "
          "which is unknown here",
          nsasm::ToString(mnemonic), target_flag);
    } else if (actual_bit == B_on && needed_bit == B_off) {
      return Error(
          "instruction %s has 16-bit immediate argument, but `%c` status flag "
          "is on here (so an 8-bit argument is required)",
          nsasm::ToString(mnemonic), target_flag);
    } else if (actual_bit == B_off && needed_bit == B_on) {
      return Error(
          "instruction %s has 8-bit immediate argument, but `%c` status flag "
          "is off here (so a 16-bit argument is required)",
          nsasm::ToString(mnemonic), target_flag);
    }
  }
  return {};
}

ErrorOr<void> Instruction::FixAddressingMode(const FlagState& flag_state) {
  BitState bs = B_unknown;
  char target_flag;
  if (addressing_mode == A_imm_fm) {
    target_flag = 'm';
    bs = flag_state.MBit();
  } else if (addressing_mode == A_imm_fx) {
    target_flag = 'x';
    bs = flag_state.XBit();
  } else {
    return {};  // nothing to fix
  }

  if (bs == B_on) {
    addressing_mode = A_imm_b;
  } else if (bs == B_off) {
    addressing_mode = A_imm_w;
  } else {
    return Error(
        "instruction %s with immediate argument depends on `%c` flag state, "
        "which is unknown here",
        nsasm::ToString(mnemonic), target_flag);
  }

  return {};
}

ErrorOr<FlagState> Instruction::Execute(const FlagState& flag_state_in) const {
  NSASM_RETURN_IF_ERROR(CheckConsistency(flag_state_in));

  // If a call has `yields` state attached, honor it
  auto yield_state = return_convention.YieldState();
  if (yield_state.has_value()) {
    return *yield_state;
  }

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
    auto arg = arg1.Evaluate(NullLookupContext());
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

int Instruction::SerializedSize() const {
  int overhead = 0;
  if (mnemonic == PM_add || mnemonic == PM_sub) {
    overhead = 1;
  }
  return InstructionLength(addressing_mode) + overhead;
}

ErrorOr<void> Instruction::Assemble(int address, const LookupContext& context,
                                    OutputSink* sink) const {
  std::uint8_t output_buf[5];
  std::uint8_t* output = output_buf;

  Mnemonic true_mnemonic = mnemonic;
  if (mnemonic == PM_add || mnemonic == PM_sub) {
    // encode a CLC (resp. SEC) before the ADC (resp. SBC) before encoding the
    // real instruction.
    *(output++) = (mnemonic == PM_add) ? 0x18 : 0x38;
    true_mnemonic = (mnemonic == PM_add) ? M_adc : M_sbc;
  }

  auto opcode = EncodeOpcode(true_mnemonic, addressing_mode);
  if (!opcode.has_value()) {
    return Error("logic error: illegal mnemonic / addressing mode pair");
  }
  *(output++) = *opcode;

  if (addressing_mode == A_imm_fm || addressing_mode == A_imm_fx) {
    return Error("logic error: side of immediate argument not known");
  }

  // Zero arguments:
  if (addressing_mode == A_imp || addressing_mode == A_acc) {
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  // One byte arguments:
  if (addressing_mode == A_imm_b || addressing_mode == A_dir_b ||
      addressing_mode == A_dir_bx || addressing_mode == A_dir_by ||
      addressing_mode == A_ind_b || addressing_mode == A_ind_bx ||
      addressing_mode == A_ind_by || addressing_mode == A_lng_b ||
      addressing_mode == A_lng_by || addressing_mode == A_stk ||
      addressing_mode == A_stk_y) {
    auto val = arg1.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val);
    *(output++) = (*val & 0xff);
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  // Two byte arguments:
  if (addressing_mode == A_imm_w || addressing_mode == A_dir_w ||
      addressing_mode == A_dir_wx || addressing_mode == A_dir_wy ||
      addressing_mode == A_ind_w || addressing_mode == A_ind_wx ||
      addressing_mode == A_lng_w) {
    auto val = arg1.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val);
    *(output++) = (*val & 0xff);
    *(output++) = ((*val >> 8) & 0xff);
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  // Three byte arguments
  if (addressing_mode == A_dir_l || addressing_mode == A_dir_lx) {
    auto val = arg1.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val);
    *(output++) = (*val & 0xff);
    *(output++) = ((*val >> 8) & 0xff);
    *(output++) = ((*val >> 16) & 0xff);
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  // source / destination
  if (addressing_mode == A_mov) {
    auto val1 = arg1.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val1);
    auto val2 = arg2.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val2);
    *(output++) = (*val2 & 0xff);
    *(output++) = (*val1 & 0xff);
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  // relative 8-bit addressing
  if (addressing_mode == A_rel8) {
    auto val1 = arg1.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val1);
    int branch_base = address + 2;
    int offset = *val1 - branch_base;
    if (offset > 127 || offset < -128) {
      return Error("Relative branch too far");
    }
    *(output++) = (offset & 0xff);
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  // relative 16-bit addressing
  if (addressing_mode == A_rel16) {
    auto val1 = arg1.Evaluate(context);
    NSASM_RETURN_IF_ERROR(val1);
    int branch_base = address + 2;
    int offset = *val1 - branch_base;
    // TODO: This is not correct; relative branching can't overflow in this
    // way.  Fix to handle wrapping on the high 8 bits.
    if (offset > 32767 || offset < -32768) {
      return Error("Relative branch too far");
    }
    *(output++) = (offset & 0xff);
    *(output++) = ((offset >> 8) & 0xff);
    return sink->Write(address, absl::MakeConstSpan(output_buf, output));
  }
  return Error("logic error: addressing mode not handled in Assemble()");
}

absl::optional<int> Instruction::FarBranchTarget(int source_address) const {
  if (addressing_mode == A_dir_l && (mnemonic == M_jmp || mnemonic == M_jsl)) {
    auto target = arg1.Evaluate(NullLookupContext());
    if (target.ok()) {
      return *target;
    }
    return absl::nullopt;
  }

  if (addressing_mode == A_dir_w && (mnemonic == M_jmp || mnemonic == M_jsr)) {
    auto target = arg1.Evaluate(NullLookupContext());
    if (target.ok()) {
      return (source_address & 0xff0000) | (*target & 0x00ffff);
    }
    return absl::nullopt;
  }

  return absl::nullopt;
}

}  // namespace nsasm
