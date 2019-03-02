#include "nsasm/addressing_mode.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string ArgsToString(AddressingMode addressing_mode, const Expression& arg1,
                         const Expression& arg2) {
  switch (addressing_mode) {
    case A_imp:
    case A_acc:
    default: {
      return "";
    }
    case A_imm_b: {
      return absl::StrFormat(" #%s", arg1.ToString(T_byte));
    }
    case A_imm_w: {
      return absl::StrFormat(" #%s", arg1.ToString(T_word));
    }
    case A_dir_b: {
      return absl::StrFormat(" %s", arg1.ToString(T_byte));
    }
    case A_dir_w: {
      return absl::StrFormat(" %s", arg1.ToString(T_word));
    }
    case A_dir_l: {
      return absl::StrFormat(" %s", arg1.ToString(T_long));
    }
    case A_dir_bx: {
      return absl::StrFormat(" %s, X", arg1.ToString(T_byte));
    }
    case A_dir_by: {
      return absl::StrFormat(" %s, Y", arg1.ToString(T_byte));
    }
    case A_dir_wx: {
      return absl::StrFormat(" %s, X", arg1.ToString(T_word));
    }
    case A_dir_wy: {
      return absl::StrFormat(" %s, Y", arg1.ToString(T_word));
    }
    case A_dir_lx: {
      return absl::StrFormat(" %s, X", arg1.ToString(T_long));
    }
    case A_ind_b: {
      return absl::StrFormat(" (%s)", arg1.ToString(T_byte));
    }
    case A_ind_w: {
      return absl::StrFormat(" (%s)", arg1.ToString(T_word));
    }
    case A_ind_bx: {
      return absl::StrFormat(" (%s, X)", arg1.ToString(T_byte));
    }
    case A_ind_by: {
      return absl::StrFormat(" (%s), Y", arg1.ToString(T_byte));
    }
    case A_ind_wx: {
      return absl::StrFormat(" (%s, X)", arg1.ToString(T_word));
    }
    case A_lng_b: {
      return absl::StrFormat(" [%s]", arg1.ToString(T_byte));
    }
    case A_lng_w: {
      return absl::StrFormat(" [%s]", arg1.ToString(T_word));
    }
    case A_lng_by: {
      return absl::StrFormat(" [%s], Y", arg1.ToString(T_byte));
    }
    case A_stk: {
      return absl::StrFormat(" %s, S", arg1.ToString(T_byte));
    }
    case A_stk_y: {
      return absl::StrFormat(" (%s, S), Y", arg1.ToString(T_byte));
    }
    case A_mov: {
      return absl::StrFormat(" #%s, #%s", arg1.ToString(T_byte),
                             arg2.ToString(T_byte));
    }
    case A_rel8:
    case A_rel16: {
      return absl::StrFormat(" %s", arg1.ToString());
    }
    case A_imm_fm:
    case A_imm_fx: {
      return absl::StrFormat(" #%s", arg1.ToString());
    }
  }
}

namespace {

ErrorOr<AddressingMode> SwitchByteWordLong(const Expression& arg,
                                           Mnemonic mnemonic,
                                           const char* mode_name,
                                           AddressingMode byte_mode,
                                           AddressingMode word_mode,
                                           AddressingMode long_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_byte) return byte_mode;
  if (numeric_type == T_word) return word_mode;
  if (numeric_type == T_long) return long_mode;
  return Error("%s address argument to %s must have explicit size", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> SwitchByteWord(const Expression& arg, Mnemonic mnemonic,
                                       const char* mode_name,
                                       AddressingMode byte_mode,
                                       AddressingMode word_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_byte) return byte_mode;
  if (numeric_type == T_word) return word_mode;
  return Error("%s address argument to %s must be a byte or word", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> SwitchWordLong(const Expression& arg, Mnemonic mnemonic,
                                       const char* mode_name,
                                       AddressingMode word_mode,
                                       AddressingMode long_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_word) return word_mode;
  if (numeric_type == T_long) return long_mode;
  return Error("%s address argument to %s must be a word or long", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> ForceByte(const Expression& arg, Mnemonic mnemonic,
                                  const char* mode_name,
                                  AddressingMode byte_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_byte) return byte_mode;
  return Error("%s address argument to %s must be a byte", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> CoerceByte(const Expression& arg, Mnemonic mnemonic,
                                   const char* mode_name,
                                   AddressingMode byte_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_byte) return byte_mode;
  if (numeric_type == T_unknown) {
    // Does decimal value fit in a byte?
    auto val = arg.Evaluate();
    if (val.ok() && (*val < 0x100 && *val >= -0x80)) {
      return byte_mode;
    }
  }
  return Error("%s argument to %s must be a byte", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> CoerceBytes(const Expression& arg1,
                                    const Expression& arg2, Mnemonic mnemonic,
                                    const char* mode_name,
                                    AddressingMode byte_mode) {
  auto r1 = CoerceByte(arg1, mnemonic, mode_name, byte_mode);
  auto r2 = CoerceByte(arg2, mnemonic, mode_name, byte_mode);
  if (r1.ok() && r2.ok()) {
    return byte_mode;
  }
  return Error("%s arguments to %s must be bytes", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> ForceWord(const Expression& arg, Mnemonic mnemonic,
                                  const char* mode_name,
                                  AddressingMode word_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_word) return word_mode;
  return Error("%s address argument to %s must be a word", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> CoerceWord(const Expression& arg, Mnemonic mnemonic,
                                   const char* mode_name,
                                   AddressingMode word_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_word) return word_mode;
  if (numeric_type == T_unknown) {
    // Does decimal value fit in a word?
    auto val = arg.Evaluate();
    if (val.ok() && (*val < 0x10000 && *val >= -0x8000)) {
      return word_mode;
    }
  }
  return Error("%s argument to %s must be a word", mode_name,
               ToString(mnemonic));
}

ErrorOr<AddressingMode> ForceLong(const Expression& arg, Mnemonic mnemonic,
                                  const char* mode_name,
                                  AddressingMode long_mode) {
  NumericType numeric_type = Unsigned(arg.Type());
  if (numeric_type == T_long) return long_mode;
  return Error("%s address argument to %s must be a long", mode_name,
               ToString(mnemonic));
}

}  // namespace

ErrorOr<AddressingMode> DeduceMode(Mnemonic m, SyntacticAddressingMode smode,
                                   const Expression& arg1,
                                   const Expression& arg2) {
  // Relative addressing is a special case.
  if (m == M_bcc || m == M_bcs || m == M_beq || m == M_bmi || m == M_bne ||
      m == M_bpl || m == M_bra || m == M_brl || m == M_bvc || m == M_bvs ||
      m == M_per) {
    if (smode != SA_dir || !arg1.SimpleIdentifier()) {
      return Error("Branch instruction %s requires a named branch target.",
                   ToString(m));
    }
    // TODO: This ought to be TakesLongOffsetArgument(), but there are
    // cyclical dep problems with that.
    return (m == M_brl || m == M_per) ? A_rel16 : A_rel8;
  }

  // Common check -- addressing modes that take addresses as arguments require
  // strictly sized arguments.
  const bool argument_is_address =
      (smode != SA_imp && smode != SA_acc && smode != SA_imm &&
       smode != SA_stk && smode != SA_stk_y && smode != SA_mov);
  if (argument_is_address && arg1.Type() == T_unknown) {
    return Error("Address argument to %s must have an explicit size",
                 ToString(m));
  }

  switch (smode) {
    case SA_imp: {
      // Six A_acc instructions (`INC A`) can be spelled without the A (`INC`).
      if (m == M_dec || m == M_inc || m == M_asl || m == M_lsr || m == M_rol ||
          m == M_ror) {
        return A_acc;
      }
      // Many other instructions take no arguments.
      if (m == M_clc || m == M_cld || m == M_cli || m == M_clv || m == M_dex ||
          m == M_dey || m == M_inx || m == M_iny || m == M_nop || m == M_pha ||
          m == M_phb || m == M_phd || m == M_phk || m == M_php || m == M_phx ||
          m == M_phy || m == M_pla || m == M_plb || m == M_pld || m == M_plp ||
          m == M_plx || m == M_ply || m == M_rti || m == M_rtl || m == M_rts ||
          m == M_sec || m == M_sed || m == M_sei || m == M_stp || m == M_tax ||
          m == M_tay || m == M_tcd || m == M_tcs || m == M_tdc || m == M_tsc ||
          m == M_tsx || m == M_txa || m == M_txs || m == M_txy || m == M_tya ||
          m == M_tyx || m == M_wai || m == M_xba || m == M_xce) {
        return A_imp;
      }
      return Error("%s requires arguments", ToString(m));
    }

    case SA_acc: {
      if (m == M_dec || m == M_inc || m == M_asl || m == M_lsr || m == M_rol ||
          m == M_ror) {
        return A_acc;
      }
      return Error("%s does not take A as an argument", ToString(m));
    }

    case SA_imm: {
      // instructions dependent on `m` bit
      if (m == M_adc || m == PM_add || m == M_and || m == M_bit || m == M_cmp ||
          m == M_eor || m == M_lda || m == M_ora || m == M_sbc || m == PM_sub) {
        return A_imm_fm;
      }

      // instructions dependent on `x` bit
      if (m == M_cpx || m == M_cpy || m == M_ldx || m == M_ldy) {
        return A_imm_fx;
      }

      if (m == M_cop || m == M_rep || m == M_sep || m == M_brk || m == M_wdm) {
        return CoerceByte(arg1, m, "Immediate", A_imm_b);
      }
      if (m == M_pea) {
        return CoerceWord(arg1, m, "Immediate", A_imm_w);
      }

      return Error("%s does not take an immediate value as an argument",
                   ToString(m));
    }

    case SA_dir: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return SwitchByteWordLong(arg1, m, "Direct", A_dir_b, A_dir_w, A_dir_l);
      }
      if (m == M_asl || m == M_bit || m == M_cpx || m == M_cpy || m == M_dec ||
          m == M_inc || m == M_ldx || m == M_ldy || m == M_lsr || m == M_rol ||
          m == M_ror || m == M_stx || m == M_sty || m == M_stz || m == M_trb ||
          m == M_tsb) {
        return SwitchByteWord(arg1, m, "Direct", A_dir_b, A_dir_w);
      }
      if (m == M_jmp) {
        return SwitchWordLong(arg1, m, "Direct", A_dir_w, A_dir_l);
      }
      if (m == M_pei) {
        return ForceByte(arg1, m, "Direct", A_dir_b);
      }
      if (m == M_jsr) {
        return ForceWord(arg1, m, "Direct", A_dir_w);
      }
      if (m == M_jsl) {
        return ForceLong(arg1, m, "Direct", A_dir_l);
      }
      return Error("%s does not take a direct address as an argument",
                   ToString(m));
    }

    case SA_dir_x: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return SwitchByteWordLong(arg1, m, "Direct indexed X", A_dir_bx,
                                  A_dir_wx, A_dir_lx);
      }
      if (m == M_asl || m == M_bit || m == M_dec || m == M_inc || m == M_ldy ||
          m == M_lsr || m == M_rol || m == M_ror || m == M_stz) {
        return SwitchByteWord(arg1, m, "Direct indexed X", A_dir_bx, A_dir_wx);
      }
      if (m == M_sty) {
        return ForceByte(arg1, m, "Direct indexed X", A_dir_bx);
      }
      return Error("%s does not take a direct indexed X address as an argument",
                   ToString(m));
    }

    case SA_dir_y: {
      if (m == M_ldx) {
        return SwitchByteWord(arg1, m, "Direct indexed Y", A_dir_by, A_dir_wy);
      }
      if (m == M_stx) {
        return ForceByte(arg1, m, "Direct indexed Y", A_dir_by);
      }
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return ForceWord(arg1, m, "Direct indexed Y", A_dir_wy);
      }
      return Error("%s does not take a direct indexed Y address as an argument",
                   ToString(m));
    }

    case SA_ind: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return ForceByte(arg1, m, "Indirect", A_ind_b);
      }
      if (m == M_jmp) {
        return ForceWord(arg1, m, "Indirect", A_ind_w);
      }
      return Error("%s does not take an indirect address as an argument",
                   ToString(m));
    }

    case SA_ind_x: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return ForceByte(arg1, m, "Indexed indirect", A_ind_bx);
      }
      if (m == M_jmp || m == M_jsr) {
        return ForceWord(arg1, m, "Indexed indirect", A_ind_wx);
      }
      return Error(
          "%s does not take an indexed indirect address as an argument",
          ToString(m));
    }

    case SA_ind_y: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return ForceByte(arg1, m, "Indirect indexed", A_ind_by);
      }
      return Error(
          "%s does not take an indirect indexed address as an argument",
          ToString(m));
    }

    case SA_lng: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return ForceByte(arg1, m, "Indirect long", A_lng_b);
      }
      if (m == M_jmp) {
        return ForceWord(arg1, m, "Indirect long", A_lng_w);
      }
      return Error("%s does not take an indirect long address as an argument",
                   ToString(m));
    }

    case SA_lng_y: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return ForceByte(arg1, m, "Indirect long indexed", A_lng_by);
      }
      return Error(
          "%s does not take an indirect long indexed address as an argument",
          ToString(m));
    }

    case SA_stk: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return CoerceByte(arg1, m, "Stack offset", A_stk);
      }
      return Error("%s does not take a stack offset as an argument",
                   ToString(m));
    }

    case SA_stk_y: {
      if (m == M_adc || m == PM_add || m == M_and || m == M_cmp || m == M_eor ||
          m == M_lda || m == M_ora || m == M_sbc || m == M_sta || m == PM_sub) {
        return CoerceByte(arg1, m, "Indirect indexed stack offset", A_stk_y);
      }
      return Error(
          "%s does not take a indirect indexed stack offset as an argument",
          ToString(m));
    }

    case SA_mov: {
      if (m == M_mvn || m == M_mvp) {
        return CoerceBytes(arg1, arg2, m, "Page", A_mov);
      }
      return Error("%s does not take two arguments", ToString(m));
    }
  }
  return Error("Logic error: bad enum value?");
}

int InstructionLength(AddressingMode a) {
  if (a == A_imp || a == A_acc) {
    return 1;
  }
  if (a == A_imm_b || a == A_dir_b || a == A_dir_bx || a == A_dir_by ||
      a == A_ind_b || a == A_ind_bx || a == A_ind_by || a == A_lng_b ||
      a == A_lng_by || a == A_stk || a == A_stk_y || a == A_rel8) {
    return 2;
  }
  if (a == A_imm_w || a == A_dir_w || a == A_dir_wx || a == A_dir_wy ||
      a == A_ind_w || a == A_ind_wx || a == A_lng_w || a == A_mov ||
      a == A_rel16) {
    return 3;
  }
  if (a == A_dir_l || a == A_dir_lx) {
    return 4;
  }
  return 0;
}

std::string ToString(AddressingMode a) {
  if (a == A_imp) return "implied";
  if (a == A_acc) return "accumulator";
  if (a == A_imm_b) return "immediate byte";
  if (a == A_imm_w) return "immediate word";
  if (a == A_dir_b) return "direct byte";
  if (a == A_dir_w) return "direct word";
  if (a == A_dir_l) return "direct long";
  if (a == A_dir_bx) return "direct X indexed byte";
  if (a == A_dir_by) return "direct Y indexed byte";
  if (a == A_dir_wx) return "direct X indexed word";
  if (a == A_dir_wy) return "direct Y indexed word";
  if (a == A_dir_lx) return "direct X indexed long";
  if (a == A_ind_b) return "indirect byte";
  if (a == A_ind_w) return "indirect word";
  if (a == A_ind_bx) return "indexed indirect X byte";
  if (a == A_ind_by) return "indirect indexed Y byte";
  if (a == A_ind_wx) return "indexed indirect X word";
  if (a == A_lng_b) return "long indirect byte";
  if (a == A_lng_w) return "long indirect word";
  if (a == A_lng_by) return "long indirect indexed Y byte";
  if (a == A_stk) return "stack relative";
  if (a == A_stk_y) return "stack relative indirect indexed Y";
  if (a == A_mov) return "source destination";
  if (a == A_rel8) return "relative 8";
  if (a == A_rel16) return "relative 16";
  if (a == A_imm_fm) return "immediate adaptive m bit";
  if (a == A_imm_fx) return "immediate adaptive x bit";
  return "???";
}

std::string ToString(SyntacticAddressingMode s) {
  if (s == SA_imp) return "implied";
  if (s == SA_acc) return "accumulator";
  if (s == SA_imm) return "immediate";
  if (s == SA_dir) return "direct";
  if (s == SA_dir_x) return "direct X indexed";
  if (s == SA_dir_y) return "direct Y indexed";
  if (s == SA_ind) return "indirect";
  if (s == SA_ind_x) return "indexed indirect X";
  if (s == SA_ind_y) return "indirect indexed Y";
  if (s == SA_lng) return "long indirect";
  if (s == SA_lng_y) return "long indirect indexed Y";
  if (s == SA_stk) return "stack relative";
  if (s == SA_stk_y) return "stack relative indirect indexed Y";
  if (s == SA_mov) return "source destination";
  return "???";
}

}  // namespace nsasm