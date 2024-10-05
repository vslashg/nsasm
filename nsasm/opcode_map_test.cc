#include "nsasm/opcode_map.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::IsEmpty;

namespace nsasm {

namespace {

// Calculating an instruction map, keyed on mnemonic.
//
// For each mnemonic, contains a second map from addressing mode to opcode.
typedef std::map<Mnemonic, std::map<AddressingMode, uint8_t>> InstructionMap;

// Eight instructions are of ALU nature, and have very similar opcode
// encodings. These also represent seven of the instructions with the
// immediate addressing mode, which respects the `m` processor status bit.
std::map<AddressingMode, uint8_t> MakeALUOp(int offset, bool is_sta = false) {
  std::map<AddressingMode, uint8_t> result;
  result[A_ind_bx] = offset + 0x01;
  result[A_stk] = offset + 0x03;
  result[A_dir_b] = offset + 0x05;
  result[A_lng_b] = offset + 0x07;
  if (!is_sta) {
    // STA writes to a location; you can't store to a constant so it does not
    // have an immediate addressing mode.
    result[A_imm_fm] = offset + 0x09;
  }
  result[A_dir_w] = offset + 0x0d;
  result[A_dir_l] = offset + 0x0f;
  result[A_ind_by] = offset + 0x11;
  result[A_ind_b] = offset + 0x12;
  result[A_stk_y] = offset + 0x13;
  result[A_dir_bx] = offset + 0x15;
  result[A_lng_by] = offset + 0x17;
  result[A_dir_wy] = offset + 0x19;
  result[A_dir_wx] = offset + 0x1d;
  result[A_dir_lx] = offset + 0x1f;
  return result;
}

// Four shift/rotate instructions have a common encoding scheme.
std::map<AddressingMode, uint8_t> MakeShiftOp(int offset) {
  std::map<AddressingMode, uint8_t> result;
  result[A_dir_b] = offset + 0x06;
  result[A_acc] = offset + 0x0a;
  result[A_dir_w] = offset + 0x0e;
  result[A_dir_bx] = offset + 0x16;
  result[A_dir_wx] = offset + 0x1e;
  return result;
}

// Two instructions for testing bits in memory share an encoding scheme.
std::map<AddressingMode, uint8_t> MakeBitTestOp(int offset) {
  std::map<AddressingMode, uint8_t> result;
  result[A_dir_b] = offset + 0x04;
  result[A_dir_w] = offset + 0x0c;
  return result;
}

// Increment/decrement share a similar encoding scheme (but the accumulator
// addressing mode does not follow the same pattern as the others.)
std::map<AddressingMode, uint8_t> MakeIncrementOp(bool increment) {
  std::map<AddressingMode, uint8_t> result;

  result[A_acc] = increment ? 0x1a : 0x3a;

  int offset = increment ? 0xe0 : 0xc0;
  result[A_dir_b] = offset + 0x06;
  result[A_dir_w] = offset + 0x0e;
  result[A_dir_bx] = offset + 0x16;
  result[A_dir_wx] = offset + 0x1e;

  return result;
}

// Two instructions load data from memory into the X or Y register.
std::map<AddressingMode, uint8_t> MakeLoadIndexOp(bool x_reg) {
  std::map<AddressingMode, uint8_t> result;
  int offset = x_reg ? 0xa2 : 0xa0;
  result[A_imm_fx] = offset + 0x00;
  result[A_dir_b] = offset + 0x04;
  result[A_dir_w] = offset + 0x0c;
  // x register commands use Y indexing, and vice versa, since indexing off
  // the register you want to manipulate would not be very useful.
  result[x_reg ? A_dir_by : A_dir_bx] = offset + 0x14;
  result[x_reg ? A_dir_wy : A_dir_wx] = offset + 0x1c;
  return result;
}

// Two instructions store the value of the X or Y register in memory.
std::map<AddressingMode, uint8_t> MakeStoreIndexOp(bool x_reg) {
  std::map<AddressingMode, uint8_t> result;
  int offset = x_reg ? 0x82 : 0x80;
  result[A_dir_b] = offset + 0x04;
  result[A_dir_w] = offset + 0x0c;
  result[x_reg ? A_dir_by : A_dir_bx] = offset + 0x14;
  return result;
}

// Two instructions compare a value against an index register
std::map<AddressingMode, uint8_t> MakeCompareIndexOp(int offset) {
  std::map<AddressingMode, uint8_t> result;
  result[A_imm_fx] = offset + 0x00;
  result[A_dir_b] = offset + 0x04;
  result[A_dir_w] = offset + 0x0c;
  return result;
}

InstructionMap MakeInstructionMap() {
  InstructionMap map;
  map[M_ora] = MakeALUOp(0x00);
  map[M_and] = MakeALUOp(0x20);
  map[M_eor] = MakeALUOp(0x40);
  map[M_adc] = MakeALUOp(0x60);
  map[M_sta] = MakeALUOp(0x80, /*is_sta=*/true);
  map[M_lda] = MakeALUOp(0xa0);
  map[M_cmp] = MakeALUOp(0xc0);
  map[M_sbc] = MakeALUOp(0xe0);

  map[M_asl] = MakeShiftOp(0x00);
  map[M_rol] = MakeShiftOp(0x20);
  map[M_lsr] = MakeShiftOp(0x40);
  map[M_ror] = MakeShiftOp(0x60);

  map[M_tsb] = MakeBitTestOp(0x00);
  map[M_trb] = MakeBitTestOp(0x10);

  map[M_inc] = MakeIncrementOp(true);
  map[M_dec] = MakeIncrementOp(false);

  map[M_ldx] = MakeLoadIndexOp(true);
  map[M_ldy] = MakeLoadIndexOp(false);

  map[M_stx] = MakeStoreIndexOp(true);
  map[M_sty] = MakeStoreIndexOp(false);

  map[M_cpx] = MakeCompareIndexOp(0xe0);
  map[M_cpy] = MakeCompareIndexOp(0xc0);

  // Branch instructions
  map[M_bcc][A_rel8] = 0x90;
  map[M_bcs][A_rel8] = 0xb0;
  map[M_beq][A_rel8] = 0xf0;
  map[M_bmi][A_rel8] = 0x30;
  map[M_bne][A_rel8] = 0xd0;
  map[M_bpl][A_rel8] = 0x10;
  map[M_bra][A_rel8] = 0x80;
  map[M_bvc][A_rel8] = 0x50;
  map[M_bvs][A_rel8] = 0x70;
  map[M_brl][A_rel16] = 0x82;

  // Jump instructions
  map[M_jmp][A_dir_w] = 0x4c;
  map[M_jmp][A_dir_l] = 0x5c;
  map[M_jmp][A_ind_w] = 0x6c;
  map[M_jmp][A_ind_wx] = 0x7c;
  map[M_jmp][A_lng_w] = 0xdc;
  map[M_jsl][A_dir_l] = 0x22;
  map[M_jsr][A_dir_w] = 0x20;
  map[M_jsr][A_ind_wx] = 0xfc;

  // Push effective address operations
  map[M_pea][A_imm_w] = 0xf4;
  map[M_pei][A_dir_b] = 0xd4;
  map[M_per][A_rel16] = 0x62;

  // BIT compares accumulator bits with a value elsewhere.  It's unique.
  map[M_bit][A_dir_b] = 0x24;
  map[M_bit][A_dir_w] = 0x2c;
  map[M_bit][A_dir_bx] = 0x34;
  map[M_bit][A_dir_wx] = 0x3c;
  map[M_bit][A_imm_fm] = 0x89;

  // STZ stores zero to a memory location; this is also unique.
  map[M_stz][A_dir_b] = 0x64;
  map[M_stz][A_dir_bx] = 0x74;
  map[M_stz][A_dir_w] = 0x9c;
  map[M_stz][A_dir_wx] = 0x9e;

  // Instructions that have only an implied addressing mode
  map[M_dex][A_imp] = 0xca;
  map[M_dey][A_imp] = 0x88;
  map[M_inx][A_imp] = 0xe8;
  map[M_iny][A_imp] = 0xc8;
  map[M_rtl][A_imp] = 0x6b;
  map[M_rts][A_imp] = 0x60;
  map[M_rti][A_imp] = 0x40;
  map[M_clc][A_imp] = 0x18;
  map[M_cld][A_imp] = 0xd8;
  map[M_cli][A_imp] = 0x58;
  map[M_clv][A_imp] = 0xb8;
  map[M_sec][A_imp] = 0x38;
  map[M_sed][A_imp] = 0xf8;
  map[M_sei][A_imp] = 0x78;
  map[M_nop][A_imp] = 0xea;
  map[M_pha][A_imp] = 0x48;
  map[M_phx][A_imp] = 0xda;
  map[M_phy][A_imp] = 0x5a;
  map[M_pla][A_imp] = 0x68;
  map[M_plx][A_imp] = 0xfa;
  map[M_ply][A_imp] = 0x7a;
  map[M_phb][A_imp] = 0x8b;
  map[M_phd][A_imp] = 0x0b;
  map[M_phk][A_imp] = 0x4b;
  map[M_php][A_imp] = 0x08;
  map[M_plb][A_imp] = 0xab;
  map[M_pld][A_imp] = 0x2b;
  map[M_plp][A_imp] = 0x28;
  map[M_stp][A_imp] = 0xdb;
  map[M_wai][A_imp] = 0xcb;
  map[M_tax][A_imp] = 0xaa;
  map[M_tay][A_imp] = 0xa8;
  map[M_tsx][A_imp] = 0xba;
  map[M_txa][A_imp] = 0x8a;
  map[M_txs][A_imp] = 0x9a;
  map[M_txy][A_imp] = 0x9b;
  map[M_tya][A_imp] = 0x98;
  map[M_tyx][A_imp] = 0xbb;
  map[M_tcd][A_imp] = 0x5b;
  map[M_tcs][A_imp] = 0x1b;
  map[M_tdc][A_imp] = 0x7b;
  map[M_tsc][A_imp] = 0x3b;
  map[M_xba][A_imp] = 0xeb;
  map[M_xce][A_imp] = 0xfb;

  // Move instructions.
  map[M_mvn][A_mov] = 0x54;
  map[M_mvp][A_mov] = 0x44;

  // Instructions that always take an immediate byte.
  //
  // Note that 6502 family assemblers traditionally treat the `BRK` instruction
  // as an implied instruction, even though it clearly takes a one-byte
  // argument. This is one convention I don't play along with.
  map[M_rep][A_imm_b] = 0xc2;
  map[M_sep][A_imm_b] = 0xe2;
  map[M_wdm][A_imm_b] = 0x42;
  map[M_cop][A_imm_b] = 0x02;
  map[M_brk][A_imm_b] = 0x00;

  return map;
}

// Test that the table of opcode families matches expectations.
//
// Independently generated from second reference source.
Family ExpectedFamily(Mnemonic mnemonic, AddressingMode mode) {
  // all 24-bit addressing modes are necessarily unique to the 65816.
  if (mode == A_dir_l || mode == A_dir_lx || mode == A_lng_b ||
      mode == A_lng_by || mode == A_lng_w) {
    return F_65816;
  }
  // Other 65816 addressing modes
  if (mode == A_stk || mode == A_stk_y || mode == A_rel16 || mode == A_mov) {
    return F_65816;
  }

  // New 65816 instructions per "programming the 65816"
  if (mnemonic == M_brl || mnemonic == M_cop || mnemonic == M_jsl ||
      mnemonic == M_mvn || mnemonic == M_mvp || mnemonic == M_pea ||
      mnemonic == M_pei || mnemonic == M_per || mnemonic == M_phb ||
      mnemonic == M_phd || mnemonic == M_plb || mnemonic == M_phk ||
      mnemonic == M_pld || mnemonic == M_rep || mnemonic == M_rtl ||
      mnemonic == M_sep || mnemonic == M_stp || mnemonic == M_tcd ||
      mnemonic == M_tcs || mnemonic == M_tdc || mnemonic == M_tsc ||
      mnemonic == M_txy || mnemonic == M_tyx || mnemonic == M_wai ||
      mnemonic == M_wdm || mnemonic == M_xba || mnemonic == M_xce) {
    return F_65816;
  }

  // JSR ($0000, X) is a 65816 extension
  if (mnemonic == M_jsr && mode == A_ind_wx) {
    return F_65816;
  }

  // 65C02 extension addressing modes
  if (mode == A_ind_b | mode == A_ind_wx) {
    return F_65C02;
  }

  // 65C02 new instructions
  if (mnemonic == M_bra || mnemonic == M_phx || mnemonic == M_phy ||
      mnemonic == M_plx || mnemonic == M_ply || mnemonic == M_stz ||
      mnemonic == M_trb || mnemonic == M_tsb) {
    return F_65C02;
  }

  // INC A and DEC A are (somehow) 65C02 extensions
  if ((mnemonic == M_inc || mnemonic == M_dec) && mode == A_acc) {
    return F_65C02;
  }

  // The 65C02 also adds three new addressing modes to BIT
  if (mnemonic == M_bit &&
      (mode == A_dir_bx || mode == A_dir_wx || mode == A_imm_fm)) {
    return F_65C02;
  }

  return F_6502;
}

}  // namespace

TEST(OpcodeMap, decode) {
  InstructionMap map = MakeInstructionMap();

  std::set<uint8_t> not_seen;
  for (int i = 0; i < 256; ++i) {
    not_seen.insert(i);
  }
  for (const auto& outer_node : map) {
    Mnemonic mnemonic = outer_node.first;
    SCOPED_TRACE(ToString(mnemonic));
    for (const auto& inner_node : outer_node.second) {
      AddressingMode addressing_mode = inner_node.first;
      uint8_t opcode = inner_node.second;
      not_seen.erase(opcode);
      SCOPED_TRACE(ToString(addressing_mode));
      SCOPED_TRACE(int(opcode));

      Mnemonic decoded_mnemonic;
      AddressingMode decoded_addressing_mode;
      std::tie(decoded_mnemonic, decoded_addressing_mode) =
          DecodeOpcode(opcode);
      EXPECT_EQ(ToString(decoded_mnemonic), ToString(mnemonic));
      EXPECT_EQ(ToString(decoded_addressing_mode), ToString(addressing_mode));
    }
  }
  EXPECT_THAT(not_seen, IsEmpty());
}

TEST(OpcodeMap, encode) {
  InstructionMap map = MakeInstructionMap();
  for (Mnemonic m : AllMnemonics()) {
    SCOPED_TRACE(ToString(m));
    auto& mnemonic_map = map[m];
    // True if this instruction takes variable-sized immediate arguments
    const bool immediate_arg_uses_m_bit = ImmediateArgumentUsesMBit(m);
    const bool immediate_arg_uses_x_bit = ImmediateArgumentUsesXBit(m);
    const bool immediate_arg_uses_status_bits =
        immediate_arg_uses_m_bit || immediate_arg_uses_x_bit;
    for (AddressingMode a : AllAddressingModes()) {
      SCOPED_TRACE(ToString(a));

      auto encoded = EncodeOpcode(m, a);
      if (m == PM_add || m == PM_sub) {
        // pseudo-mnemonics don't have encodings
        EXPECT_FALSE(encoded.has_value());
        continue;
      }

      auto it = mnemonic_map.find(a);
      if (immediate_arg_uses_status_bits && (a == A_imm_b || a == A_imm_w)) {
        // A_imm_b and A_imm_w are valid flags modes for this instruction,
        // but they appear in this test's InstructionMap under their variable-
        // sized form.
        if (immediate_arg_uses_m_bit) {
          it = mnemonic_map.find(A_imm_fm);
        } else if (immediate_arg_uses_x_bit) {
          it = mnemonic_map.find(A_imm_fx);
        }
      }
      if (it == mnemonic_map.end()) {
        EXPECT_FALSE(encoded.has_value());
      } else {
        EXPECT_TRUE(encoded.has_value());
        if (encoded.has_value()) {
          EXPECT_EQ(*encoded, it->second);
        }
      }
    }
  }
}

TEST(OpcodeMap, controlling_flag) {
  // Simple sanity check of controlling operation map.  An instruction with
  // operand A_imm_fx is governed by the X flag, and similarly A_imm_fm is
  // governed by the M flag.
  for (int i = 0; i < 256; ++i) {
    std::pair<Mnemonic, AddressingMode> decoded = DecodeOpcode(i);
    if (decoded.second == A_imm_fx) {
      EXPECT_EQ(FlagControllingInstructionSize(decoded.first), kUsesXFlag);
    } else if (decoded.second == A_imm_fm) {
      EXPECT_EQ(FlagControllingInstructionSize(decoded.first), kUsesMFlag);
    }
  }
}

TEST(OpcodeMap, processor_family) {
  // Check that each opcode is assigned to the correct family, using
  for (int i = 0; i < 256; ++i) {
    auto decoded = DecodeOpcode(i);
    SCOPED_TRACE(i);
    SCOPED_TRACE(ToString(decoded.first));
    SCOPED_TRACE(ToString(decoded.second));
    Family expected = ExpectedFamily(decoded.first, decoded.second);
    Family got = FamilyForOpcode(i);
    EXPECT_EQ(expected, got);
  }
}

}  // namespace nsasm
