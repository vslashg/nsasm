#include "nsasm/instruction.h"

#include "absl/strings/str_format.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

std::string Instruction::ToString() const {
  return absl::StrFormat("%s%s%s", nsasm::ToString(mnemonic),
                         ArgsToString(addressing_mode, arg1, arg2),
                         return_convention.ToSuffixString());
}

ErrorOr<void> Instruction::CheckConsistency(
    const StatusFlags& status_flags) const {
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
    if (status_flags.MBit() != B_on && status_flags.MBit() != B_off) {
      return Error(
          "instruction %s with immediate argument depends on `m` flag state, "
          "which is unknown here",
          nsasm::ToString(mnemonic));
    }
  } else if (addressing_mode == A_imm_fx) {
    // as above, but for the `x` bit
    if (status_flags.XBit() != B_on && status_flags.XBit() != B_off) {
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
      actual_bit = status_flags.MBit();
    } else if (ImmediateArgumentUsesXBit(mnemonic)) {
      target_flag = 'x';
      actual_bit = status_flags.XBit();
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

ErrorOr<void> Instruction::FixAddressingMode(const StatusFlags& status_flags) {
  BitState bs = B_unknown;
  char target_flag;
  if (addressing_mode == A_imm_fm) {
    target_flag = 'm';
    bs = status_flags.MBit();
  } else if (addressing_mode == A_imm_fx) {
    target_flag = 'x';
    bs = status_flags.XBit();
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

ErrorOr<void> Instruction::Execute(ExecutionState* es, absl::optional<int> pc,
                                   const LookupContext& context,
                                   bool* needs_reeval) const {
  NSASM_RETURN_IF_ERROR(CheckConsistency(es->Flags()));

  auto maybe_arg1 = [&context, needs_reeval, this]() -> absl::optional<int> {
    auto arg = arg1.Evaluate(context);
    if (!arg.ok()) {
      if (needs_reeval) {
        *needs_reeval = true;
      }
      return absl::nullopt;
    }
    return *arg;
  };

  auto maybe_arg2 = [&context, needs_reeval, this]() -> absl::optional<int> {
    auto arg = arg2.Evaluate(context);
    if (!arg.ok()) {
      if (needs_reeval) {
        *needs_reeval = true;
      }
      return absl::nullopt;
    }
    return *arg;
  };

  const Mnemonic& m = mnemonic;

  // Instructions that clear or set carry bit (used to prime the XCE
  // instruction, which swaps the carry bit and emulation bit.)

  // BCC and BCS essentially set and clear the c bit for the next instruction,
  // respectively, because if the bit is in the opposite state, we will branch
  // instead.
  if (m == M_sec || m == M_bcc) {
    es->Flags().SetCBit(B_on);
    return {};
  } else if (m == M_clc || m == M_bcs) {
    es->Flags().SetCBit(B_off);
    return {};
  }

  // Instructions that clear or set status bits explicitly
  if (m == M_rep || m == M_sep) {
    BitState target = (m == M_rep) ? B_off : B_on;
    auto arg = maybe_arg1();
    if (!arg.has_value()) {
      // If REP or SEP are invoked with an unknown argument (a constant pulled
      // from another module, say), we will have to account for the ambiguity.
      //
      // Each bit will either be set to `target` or else left alone.  If the
      // current value of a bit is equal to `target`, it's unchanged; otherwise
      // it becomes ambiguous.
      if (es->Flags().CBit() != target) {
        es->Flags().SetCBit(B_unknown);
      }
      if (es->Flags().XBit() != target) {
        es->Flags().SetXBit(B_unknown);
      }
      if (es->Flags().MBit() != target) {
        es->Flags().SetMBit(B_unknown);
      }
      return {};
    }

    // If the argument is known, we can set the affected bits.
    if (*arg & 0x01) {
      es->Flags().SetCBit(target);
    }
    if (*arg & 0x10) {
      es->Flags().SetXBit(target);
    }
    if (*arg & 0x20) {
      es->Flags().SetMBit(target);
    }
    return {};
  }

  // ALU operations which modify the accumulator and modify the carry bit.
  // Static analysis could attempt to track the math when both inputs
  // are known, but this would require tracking the state of the BCD
  // mode.  This would add complexity to the implementation and to input
  // assembly, for no real-world gains.
  if (m == M_adc || m == M_sbc || m == PM_add || m == PM_sub || m == M_asl ||
      m == M_lsr || m == M_rol || m == M_ror) {
    es->WipeAccumulator();
    es->WipeCarry();
    return {};
  }

  // Comparison operations which modify the carry bit.
  if (m == M_cmp || m == M_cpx || m == M_cpy) {
    es->WipeCarry();
    return {};
  }

  // Bitwise operations which modify the accumulator but don't touch the
  // carry bit.
  if (m == M_and || m == M_eor || m == M_ora) {
    es->WipeAccumulator();
    return {};
  }

  // Increment and decrement
  if (m == M_inc || m == M_dec || m == M_inx || m == M_dex || m == M_iny ||
      m == M_dey) {
    if (addressing_mode != A_acc && addressing_mode != A_imp) {
      // incrementing/decrementing memory does not affect our tracked state.
      return {};
    }
    RegisterValue* reg;
    int mask = 0xffff;
    if (m == M_inc || m == M_dec) {
      reg = &es->Accumulator();
      if (es->Flags().MBit() == B_on) mask = 0xff;
    } else if (m == M_inx || m == M_dex) {
      reg = &es->XRegister();
      if (es->Flags().XBit() == B_on) mask = 0xff;
    } else {
      assert(m == M_iny || m == M_dey);
      reg = &es->YRegister();
      if (es->Flags().XBit() == B_on) mask = 0xff;
    }
    int offset = (m == M_dec || m == M_dex || m == M_dey) ? -1 : 1;
    reg->Add(offset, mask);
    return {};
  }

  if (m == M_lda || m == M_ldx || m == M_ldy) {
    if (addressing_mode == A_imm_b || addressing_mode == A_imm_fx ||
        addressing_mode == A_imm_fm || addressing_mode == A_imm_w) {
      RegisterValue* reg;
      if (m == M_lda) {
        reg = &es->Accumulator();
      } else if (m == M_ldx) {
        reg = &es->XRegister();
      } else {
        reg = &es->YRegister();
      }
      auto arg = maybe_arg1();
      if (!arg.has_value()) {
        *reg = RegisterValue();
      } else {
        *reg = *arg;
      }
    }
    return {};
  }

  if (m == M_mvn || m == M_mvp) {
    auto initial_a = maybe_arg1();
    if (es->Flags().MBit() == B_on) {
      es->Accumulator() = 0xff;
    } else if (es->Flags().MBit() == B_off) {
      es->Accumulator() = 0xffff;
    } else {
      es->WipeAccumulator();
    }
    if (!initial_a.has_value()) {
      es->XRegister() = RegisterValue();
      es->YRegister() = RegisterValue();
    } else {
      // X and Y are adjusted per the accumulator
      int mask = (es->Flags().XBit() == B_on) ? 0xff : 0xffff;
      int offset = (m == M_mvn) ? 1 + *initial_a : -1 - *initial_a;
      es->XRegister().Add(offset, mask);
      es->YRegister().Add(offset, mask);
    }
    auto dbr = maybe_arg2();
    if (dbr.has_value()) {
      es->DataBankRegister() = *dbr;
    } else {
      es->DataBankRegister() = RegisterValue();
    }
    return {};
  }

  if (m == M_pea) {
    es->GetStack().PushWord(maybe_arg1());
    return {};
  }

  if (m == M_pei || m == M_per) {
    // We have the data needed to implement PER, but it's not clear that doing
    // so would be helpful.
    es->GetStack().PushUnknownWord();
    return {};
  }

  if (m == M_pha) {
    es->PushAccumulator();
    return {};
  }

  if (m == M_phx) {
    es->PushXRegister();
    return {};
  }

  if (m == M_phy) {
    es->PushYRegister();
    return {};
  }

  if (m == M_pla) {
    es->PullAccumulator();
    return {};
  }

  if (m == M_plx) {
    es->PullXRegister();
    return {};
  }

  if (m == M_ply) {
    es->PullYRegister();
    return {};
  }

  // Instructions that push or pull the status bits onto the stack.
  if (m == M_php) {
    es->PushFlags();
    return {};
  } else if (m == M_plp) {
    es->PullFlags();
    return {};
  }

  // Instruction that swaps the c and e bits.  This can change the
  // m and x bits as a side effect.
  if (m == M_xce) {
    es->Flags().ExchangeCE();
    return {};
  }

  // Subroutine and interrupt calls.  For now we assume these always clobber
  // the carry bit, though we could always add an exception to this to the
  // ReturnConvention state if it proves necessary.
  if (m == M_jmp || m == M_jsl || m == M_jsr || m == M_brk || m == M_cop) {
    // If a call has `yields` state attached, honor it
    auto yield_flags = return_convention.YieldFlags();
    if (yield_flags.has_value()) {
      es->Flags() = *yield_flags;
    }
    es->Flags().SetCBit(B_unknown);
    return {};
  }

  // Other instructions don't effect the flag state.
  return {};
}

ErrorOr<void> Instruction::ExecuteBranch(ExecutionState* es) const {
  NSASM_RETURN_IF_ERROR(Execute(es));
  if (mnemonic == M_bcc) {
    es->Flags().SetCBit(B_off);
  } else if (mnemonic == M_bcs) {
    es->Flags().SetCBit(B_on);
  }
  return {};
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
    int branch_base = address + 3;
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
