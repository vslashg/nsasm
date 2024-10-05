#include "nsasm/opcode_map.h"

#include <algorithm>
#include <map>

#include "absl/container/flat_hash_map.h"

namespace nsasm {

namespace {

struct DecodeMapEntry {
  Mnemonic mnemonic;
  AddressingMode mode;
  Family family;
};

using EncodeMapKey = std::pair<Mnemonic, AddressingMode>;

constexpr DecodeMapEntry decode_map[256] = {
    {M_brk, A_imm_b, F_6502},    // 0x00
    {M_ora, A_ind_bx, F_6502},   // 0x01
    {M_cop, A_imm_b, F_65816},   // 0x02
    {M_ora, A_stk, F_65816},     // 0x03
    {M_tsb, A_dir_b, F_65C02},   // 0x04
    {M_ora, A_dir_b, F_6502},    // 0x05
    {M_asl, A_dir_b, F_6502},    // 0x06
    {M_ora, A_lng_b, F_65816},   // 0x07
    {M_php, A_imp, F_6502},      // 0x08
    {M_ora, A_imm_fm, F_6502},   // 0x09
    {M_asl, A_acc, F_6502},      // 0x0a
    {M_phd, A_imp, F_65816},     // 0x0b
    {M_tsb, A_dir_w, F_65C02},   // 0x0c
    {M_ora, A_dir_w, F_6502},    // 0x0d
    {M_asl, A_dir_w, F_6502},    // 0x0e
    {M_ora, A_dir_l, F_65816},   // 0x0f
    {M_bpl, A_rel8, F_6502},     // 0x10
    {M_ora, A_ind_by, F_6502},   // 0x11
    {M_ora, A_ind_b, F_65C02},   // 0x12
    {M_ora, A_stk_y, F_65816},   // 0x13
    {M_trb, A_dir_b, F_65C02},   // 0x14
    {M_ora, A_dir_bx, F_6502},   // 0x15
    {M_asl, A_dir_bx, F_6502},   // 0x16
    {M_ora, A_lng_by, F_65816},  // 0x17
    {M_clc, A_imp, F_6502},      // 0x18
    {M_ora, A_dir_wy, F_6502},   // 0x19
    {M_inc, A_acc, F_65C02},     // 0x1a
    {M_tcs, A_imp, F_65816},     // 0x1b
    {M_trb, A_dir_w, F_65C02},   // 0x1c
    {M_ora, A_dir_wx, F_6502},   // 0x1d
    {M_asl, A_dir_wx, F_6502},   // 0x1e
    {M_ora, A_dir_lx, F_65816},  // 0x1f
    {M_jsr, A_dir_w, F_6502},    // 0x20
    {M_and, A_ind_bx, F_6502},   // 0x21
    {M_jsl, A_dir_l, F_65816},   // 0x22
    {M_and, A_stk, F_65816},     // 0x23
    {M_bit, A_dir_b, F_6502},    // 0x24
    {M_and, A_dir_b, F_6502},    // 0x25
    {M_rol, A_dir_b, F_6502},    // 0x26
    {M_and, A_lng_b, F_65816},   // 0x27
    {M_plp, A_imp, F_6502},      // 0x28
    {M_and, A_imm_fm, F_6502},   // 0x29
    {M_rol, A_acc, F_6502},      // 0x2a
    {M_pld, A_imp, F_65816},     // 0x2b
    {M_bit, A_dir_w, F_6502},    // 0x2c
    {M_and, A_dir_w, F_6502},    // 0x2d
    {M_rol, A_dir_w, F_6502},    // 0x2e
    {M_and, A_dir_l, F_65816},   // 0x2f
    {M_bmi, A_rel8, F_6502},     // 0x30
    {M_and, A_ind_by, F_6502},   // 0x31
    {M_and, A_ind_b, F_65C02},   // 0x32
    {M_and, A_stk_y, F_65816},   // 0x33
    {M_bit, A_dir_bx, F_65C02},  // 0x34
    {M_and, A_dir_bx, F_6502},   // 0x35
    {M_rol, A_dir_bx, F_6502},   // 0x36
    {M_and, A_lng_by, F_65816},  // 0x37
    {M_sec, A_imp, F_6502},      // 0x38
    {M_and, A_dir_wy, F_6502},   // 0x39
    {M_dec, A_acc, F_65C02},     // 0x3a
    {M_tsc, A_imp, F_65816},     // 0x3b
    {M_bit, A_dir_wx, F_65C02},  // 0x3c
    {M_and, A_dir_wx, F_6502},   // 0x3d
    {M_rol, A_dir_wx, F_6502},   // 0x3e
    {M_and, A_dir_lx, F_65816},  // 0x3f
    {M_rti, A_imp, F_6502},      // 0x40
    {M_eor, A_ind_bx, F_6502},   // 0x41
    {M_wdm, A_imm_b, F_65816},   // 0x42
    {M_eor, A_stk, F_65816},     // 0x43
    {M_mvp, A_mov, F_65816},     // 0x44
    {M_eor, A_dir_b, F_6502},    // 0x45
    {M_lsr, A_dir_b, F_6502},    // 0x46
    {M_eor, A_lng_b, F_65816},   // 0x47
    {M_pha, A_imp, F_6502},      // 0x48
    {M_eor, A_imm_fm, F_6502},   // 0x49
    {M_lsr, A_acc, F_6502},      // 0x4a
    {M_phk, A_imp, F_65816},     // 0x4b
    {M_jmp, A_dir_w, F_6502},    // 0x4c
    {M_eor, A_dir_w, F_6502},    // 0x4d
    {M_lsr, A_dir_w, F_6502},    // 0x4e
    {M_eor, A_dir_l, F_65816},   // 0x4f
    {M_bvc, A_rel8, F_6502},     // 0x50
    {M_eor, A_ind_by, F_6502},   // 0x51
    {M_eor, A_ind_b, F_65C02},   // 0x52
    {M_eor, A_stk_y, F_65816},   // 0x53
    {M_mvn, A_mov, F_65816},     // 0x54
    {M_eor, A_dir_bx, F_6502},   // 0x55
    {M_lsr, A_dir_bx, F_6502},   // 0x56
    {M_eor, A_lng_by, F_65816},  // 0x57
    {M_cli, A_imp, F_6502},      // 0x58
    {M_eor, A_dir_wy, F_6502},   // 0x59
    {M_phy, A_imp, F_65C02},     // 0x5a
    {M_tcd, A_imp, F_65816},     // 0x5b
    {M_jmp, A_dir_l, F_65816},   // 0x5c
    {M_eor, A_dir_wx, F_6502},   // 0x5d
    {M_lsr, A_dir_wx, F_6502},   // 0x5e
    {M_eor, A_dir_lx, F_65816},  // 0x5f
    {M_rts, A_imp, F_6502},      // 0x60
    {M_adc, A_ind_bx, F_6502},   // 0x61
    {M_per, A_rel16, F_65816},   // 0x62
    {M_adc, A_stk, F_65816},     // 0x63
    {M_stz, A_dir_b, F_65C02},   // 0x64
    {M_adc, A_dir_b, F_6502},    // 0x65
    {M_ror, A_dir_b, F_6502},    // 0x66
    {M_adc, A_lng_b, F_65816},   // 0x67
    {M_pla, A_imp, F_6502},      // 0x68
    {M_adc, A_imm_fm, F_6502},   // 0x69
    {M_ror, A_acc, F_6502},      // 0x6a
    {M_rtl, A_imp, F_65816},     // 0x6b
    {M_jmp, A_ind_w, F_6502},    // 0x6c
    {M_adc, A_dir_w, F_6502},    // 0x6d
    {M_ror, A_dir_w, F_6502},    // 0x6e
    {M_adc, A_dir_l, F_65816},   // 0x6f
    {M_bvs, A_rel8, F_6502},     // 0x70
    {M_adc, A_ind_by, F_6502},   // 0x71
    {M_adc, A_ind_b, F_65C02},   // 0x72
    {M_adc, A_stk_y, F_65816},   // 0x73
    {M_stz, A_dir_bx, F_65C02},  // 0x74
    {M_adc, A_dir_bx, F_6502},   // 0x75
    {M_ror, A_dir_bx, F_6502},   // 0x76
    {M_adc, A_lng_by, F_65816},  // 0x77
    {M_sei, A_imp, F_6502},      // 0x78
    {M_adc, A_dir_wy, F_6502},   // 0x79
    {M_ply, A_imp, F_65C02},     // 0x7a
    {M_tdc, A_imp, F_65816},     // 0x7b
    {M_jmp, A_ind_wx, F_65C02},  // 0x7c
    {M_adc, A_dir_wx, F_6502},   // 0x7d
    {M_ror, A_dir_wx, F_6502},   // 0x7e
    {M_adc, A_dir_lx, F_65816},  // 0x7f
    {M_bra, A_rel8, F_65C02},    // 0x80
    {M_sta, A_ind_bx, F_6502},   // 0x81
    {M_brl, A_rel16, F_65816},   // 0x82
    {M_sta, A_stk, F_65816},     // 0x83
    {M_sty, A_dir_b, F_6502},    // 0x84
    {M_sta, A_dir_b, F_6502},    // 0x85
    {M_stx, A_dir_b, F_6502},    // 0x86
    {M_sta, A_lng_b, F_65816},   // 0x87
    {M_dey, A_imp, F_6502},      // 0x88
    {M_bit, A_imm_fm, F_65C02},  // 0x89
    {M_txa, A_imp, F_6502},      // 0x8a
    {M_phb, A_imp, F_65816},     // 0x8b
    {M_sty, A_dir_w, F_6502},    // 0x8c
    {M_sta, A_dir_w, F_6502},    // 0x8d
    {M_stx, A_dir_w, F_6502},    // 0x8e
    {M_sta, A_dir_l, F_65816},   // 0x8f
    {M_bcc, A_rel8, F_6502},     // 0x90
    {M_sta, A_ind_by, F_6502},   // 0x91
    {M_sta, A_ind_b, F_65C02},   // 0x92
    {M_sta, A_stk_y, F_65816},   // 0x93
    {M_sty, A_dir_bx, F_6502},   // 0x94
    {M_sta, A_dir_bx, F_6502},   // 0x95
    {M_stx, A_dir_by, F_6502},   // 0x96
    {M_sta, A_lng_by, F_65816},  // 0x97
    {M_tya, A_imp, F_6502},      // 0x98
    {M_sta, A_dir_wy, F_6502},   // 0x99
    {M_txs, A_imp, F_6502},      // 0x9a
    {M_txy, A_imp, F_65816},     // 0x9b
    {M_stz, A_dir_w, F_65C02},   // 0x9c
    {M_sta, A_dir_wx, F_6502},   // 0x9d
    {M_stz, A_dir_wx, F_65C02},  // 0x9e
    {M_sta, A_dir_lx, F_65816},  // 0x9f
    {M_ldy, A_imm_fx, F_6502},   // 0xa0
    {M_lda, A_ind_bx, F_6502},   // 0xa1
    {M_ldx, A_imm_fx, F_6502},   // 0xa2
    {M_lda, A_stk, F_65816},     // 0xa3
    {M_ldy, A_dir_b, F_6502},    // 0xa4
    {M_lda, A_dir_b, F_6502},    // 0xa5
    {M_ldx, A_dir_b, F_6502},    // 0xa6
    {M_lda, A_lng_b, F_65816},   // 0xa7
    {M_tay, A_imp, F_6502},      // 0xa8
    {M_lda, A_imm_fm, F_6502},   // 0xa9
    {M_tax, A_imp, F_6502},      // 0xaa
    {M_plb, A_imp, F_65816},     // 0xab
    {M_ldy, A_dir_w, F_6502},    // 0xac
    {M_lda, A_dir_w, F_6502},    // 0xad
    {M_ldx, A_dir_w, F_6502},    // 0xae
    {M_lda, A_dir_l, F_65816},   // 0xaf
    {M_bcs, A_rel8, F_6502},     // 0xb0
    {M_lda, A_ind_by, F_6502},   // 0xb1
    {M_lda, A_ind_b, F_65C02},   // 0xb2
    {M_lda, A_stk_y, F_65816},   // 0xb3
    {M_ldy, A_dir_bx, F_6502},   // 0xb4
    {M_lda, A_dir_bx, F_6502},   // 0xb5
    {M_ldx, A_dir_by, F_6502},   // 0xb6
    {M_lda, A_lng_by, F_65816},  // 0xb7
    {M_clv, A_imp, F_6502},      // 0xb8
    {M_lda, A_dir_wy, F_6502},   // 0xb9
    {M_tsx, A_imp, F_6502},      // 0xba
    {M_tyx, A_imp, F_65816},     // 0xbb
    {M_ldy, A_dir_wx, F_6502},   // 0xbc
    {M_lda, A_dir_wx, F_6502},   // 0xbd
    {M_ldx, A_dir_wy, F_6502},   // 0xbe
    {M_lda, A_dir_lx, F_65816},  // 0xbf
    {M_cpy, A_imm_fx, F_6502},   // 0xc0
    {M_cmp, A_ind_bx, F_6502},   // 0xc1
    {M_rep, A_imm_b, F_65816},   // 0xc2
    {M_cmp, A_stk, F_65816},     // 0xc3
    {M_cpy, A_dir_b, F_6502},    // 0xc4
    {M_cmp, A_dir_b, F_6502},    // 0xc5
    {M_dec, A_dir_b, F_6502},    // 0xc6
    {M_cmp, A_lng_b, F_65816},   // 0xc7
    {M_iny, A_imp, F_6502},      // 0xc8
    {M_cmp, A_imm_fm, F_6502},   // 0xc9
    {M_dex, A_imp, F_6502},      // 0xca
    {M_wai, A_imp, F_65816},     // 0xcb
    {M_cpy, A_dir_w, F_6502},    // 0xcc
    {M_cmp, A_dir_w, F_6502},    // 0xcd
    {M_dec, A_dir_w, F_6502},    // 0xce
    {M_cmp, A_dir_l, F_65816},   // 0xcf
    {M_bne, A_rel8, F_6502},     // 0xd0
    {M_cmp, A_ind_by, F_6502},   // 0xd1
    {M_cmp, A_ind_b, F_65C02},   // 0xd2
    {M_cmp, A_stk_y, F_65816},   // 0xd3
    {M_pei, A_dir_b, F_65816},   // 0xd4
    {M_cmp, A_dir_bx, F_6502},   // 0xd5
    {M_dec, A_dir_bx, F_6502},   // 0xd6
    {M_cmp, A_lng_by, F_65816},  // 0xd7
    {M_cld, A_imp, F_6502},      // 0xd8
    {M_cmp, A_dir_wy, F_6502},   // 0xd9
    {M_phx, A_imp, F_65C02},     // 0xda
    {M_stp, A_imp, F_65816},     // 0xdb
    {M_jmp, A_lng_w, F_65816},   // 0xdc
    {M_cmp, A_dir_wx, F_6502},   // 0xdd
    {M_dec, A_dir_wx, F_6502},   // 0xde
    {M_cmp, A_dir_lx, F_65816},  // 0xdf
    {M_cpx, A_imm_fx, F_6502},   // 0xe0
    {M_sbc, A_ind_bx, F_6502},   // 0xe1
    {M_sep, A_imm_b, F_65816},   // 0xe2
    {M_sbc, A_stk, F_65816},     // 0xe3
    {M_cpx, A_dir_b, F_6502},    // 0xe4
    {M_sbc, A_dir_b, F_6502},    // 0xe5
    {M_inc, A_dir_b, F_6502},    // 0xe6
    {M_sbc, A_lng_b, F_65816},   // 0xe7
    {M_inx, A_imp, F_6502},      // 0xe8
    {M_sbc, A_imm_fm, F_6502},   // 0xe9
    {M_nop, A_imp, F_6502},      // 0xea
    {M_xba, A_imp, F_65816},     // 0xeb
    {M_cpx, A_dir_w, F_6502},    // 0xec
    {M_sbc, A_dir_w, F_6502},    // 0xed
    {M_inc, A_dir_w, F_6502},    // 0xee
    {M_sbc, A_dir_l, F_65816},   // 0xef
    {M_beq, A_rel8, F_6502},     // 0xf0
    {M_sbc, A_ind_by, F_6502},   // 0xf1
    {M_sbc, A_ind_b, F_65C02},   // 0xf2
    {M_sbc, A_stk_y, F_65816},   // 0xf3
    {M_pea, A_imm_w, F_65816},   // 0xf4
    {M_sbc, A_dir_bx, F_6502},   // 0xf5
    {M_inc, A_dir_bx, F_6502},   // 0xf6
    {M_sbc, A_lng_by, F_65816},  // 0xf7
    {M_sed, A_imp, F_6502},      // 0xf8
    {M_sbc, A_dir_wy, F_6502},   // 0xf9
    {M_plx, A_imp, F_65C02},     // 0xfa
    {M_xce, A_imp, F_65816},     // 0xfb
    {M_jsr, A_ind_wx, F_65816},  // 0xfc
    {M_sbc, A_dir_wx, F_6502},   // 0xfd
    {M_inc, A_dir_wx, F_6502},   // 0xfe
    {M_sbc, A_dir_lx, F_65816},  // 0xff
};

absl::flat_hash_map<EncodeMapKey, uint8_t> MakeReverseOpcodeMap() {
  absl::flat_hash_map<EncodeMapKey, uint8_t> reverse;
  for (int i = 0; i < 256; ++i) {
    EncodeMapKey entry = {decode_map[i].mnemonic, decode_map[i].mode};
    reverse[entry] = i;
    if (entry.second == A_imm_fm || entry.second == A_imm_fx) {
      // add reverse lookup entries for the non-sentinel immediate modes
      entry.second = A_imm_b;
      reverse[entry] = i;
      entry.second = A_imm_w;
      reverse[entry] = i;
    }
  }
  return reverse;
}

const absl::flat_hash_map<EncodeMapKey, uint8_t>& ReverseOpcodeMap() {
  static auto* map =
      new absl::flat_hash_map<EncodeMapKey, uint8_t>(MakeReverseOpcodeMap());
  return *map;
}

}  // namespace

std::string ToString(Family f) {
  switch (f) {
    case F_6502:
      return "6502";
    case F_65C02:
      return "65C02";
    case F_65816:
      return "65816";
    default:
      return "???";
  }
}

std::pair<Mnemonic, AddressingMode> DecodeOpcode(uint8_t opcode) {
  return {decode_map[opcode].mnemonic, decode_map[opcode].mode};
}

Family FamilyForOpcode(uint8_t opcode) {
  return decode_map[opcode].family;
}

absl::optional<std::uint8_t> EncodeOpcode(Mnemonic m, AddressingMode a) {
  const auto& map = ReverseOpcodeMap();
  auto it = map.find(std::make_pair(m, a));
  if (it == map.end()) {
    return absl::nullopt;
  }
  return it->second;
}

bool ImmediateArgumentUsesMBit(Mnemonic m) {
  EncodeMapKey e(m, A_imm_fm);
  return ReverseOpcodeMap().contains(e) || m == PM_add || m == PM_sub;
}

bool ImmediateArgumentUsesXBit(Mnemonic m) {
  EncodeMapKey e(m, A_imm_fx);
  return ReverseOpcodeMap().contains(e);
}

bool TakesOffsetArgument(Mnemonic m) {
  EncodeMapKey e8(m, A_rel8);
  EncodeMapKey e16(m, A_rel16);
  return ReverseOpcodeMap().contains(e8) || ReverseOpcodeMap().contains(e16);
}

bool TakesLongOffsetArgument(Mnemonic m) {
  EncodeMapKey e16(m, A_rel16);
  return ReverseOpcodeMap().contains(e16);
}

bool IsLegalCombination(Mnemonic m, AddressingMode a) {
  auto opcode_it = ReverseOpcodeMap().find(std::make_pair(m, a));
  return opcode_it != ReverseOpcodeMap().end();
}

StatusFlagUsed FlagControllingInstructionSize(Mnemonic m) {
  // Accumulator-width instructions
  if (m == M_adc || m == PM_add || m == M_and || m == M_asl || m == M_bit ||
      m == M_cmp || m == M_dec || m == M_eor || m == M_inc || m == M_lda ||
      m == M_lsr || m == M_ora || m == M_pha || m == M_pla || m == M_rol ||
      m == M_ror || m == M_sbc || m == M_sta || m == M_stz || m == PM_sub ||
      m == M_trb || m == M_tsb || m == M_txa || m == M_tya) {
    return kUsesMFlag;
  }
  // Index-register-width instructions
  if (m == M_cpx || m == M_cpy || m == M_dex || m == M_dey || m == M_inx ||
      m == M_iny || m == M_ldx || m == M_ldy || m == M_phx || m == M_phy ||
      m == M_plx || m == M_ply || m == M_stx || m == M_sty || m == M_tax ||
      m == M_tay || m == M_tsx || m == M_txy || m == M_tyx) {
    return kUsesXFlag;
  }
  return kNotVariable;
}

}  // namespace nsasm
