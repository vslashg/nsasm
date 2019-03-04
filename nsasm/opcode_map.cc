#include "nsasm/opcode_map.h"

#include <algorithm>
#include <map>

#include "absl/container/flat_hash_map.h"

namespace nsasm {

namespace {

using DecodeMapEntry = std::pair<Mnemonic, AddressingMode>;

constexpr DecodeMapEntry decode_map[256] = {
    {M_brk, A_imm_b},   // 0x00
    {M_ora, A_ind_bx},  // 0x01
    {M_cop, A_imm_b},   // 0x02
    {M_ora, A_stk},     // 0x03
    {M_tsb, A_dir_b},   // 0x04
    {M_ora, A_dir_b},   // 0x05
    {M_asl, A_dir_b},   // 0x06
    {M_ora, A_lng_b},   // 0x07
    {M_php, A_imp},     // 0x08
    {M_ora, A_imm_fm},  // 0x09
    {M_asl, A_acc},     // 0x0a
    {M_phd, A_imp},     // 0x0b
    {M_tsb, A_dir_w},   // 0x0c
    {M_ora, A_dir_w},   // 0x0d
    {M_asl, A_dir_w},   // 0x0e
    {M_ora, A_dir_l},   // 0x0f
    {M_bpl, A_rel8},    // 0x10
    {M_ora, A_ind_by},  // 0x11
    {M_ora, A_ind_b},   // 0x12
    {M_ora, A_stk_y},   // 0x13
    {M_trb, A_dir_b},   // 0x14
    {M_ora, A_dir_bx},  // 0x15
    {M_asl, A_dir_bx},  // 0x16
    {M_ora, A_lng_by},  // 0x17
    {M_clc, A_imp},     // 0x18
    {M_ora, A_dir_wy},  // 0x19
    {M_inc, A_acc},     // 0x1a
    {M_tcs, A_imp},     // 0x1b
    {M_trb, A_dir_w},   // 0x1c
    {M_ora, A_dir_wx},  // 0x1d
    {M_asl, A_dir_wx},  // 0x1e
    {M_ora, A_dir_lx},  // 0x1f
    {M_jsr, A_dir_w},   // 0x20
    {M_and, A_ind_bx},  // 0x21
    {M_jsl, A_dir_l},   // 0x22
    {M_and, A_stk},     // 0x23
    {M_bit, A_dir_b},   // 0x24
    {M_and, A_dir_b},   // 0x25
    {M_rol, A_dir_b},   // 0x26
    {M_and, A_lng_b},   // 0x27
    {M_plp, A_imp},     // 0x28
    {M_and, A_imm_fm},  // 0x29
    {M_rol, A_acc},     // 0x2a
    {M_pld, A_imp},     // 0x2b
    {M_bit, A_dir_w},   // 0x2c
    {M_and, A_dir_w},   // 0x2d
    {M_rol, A_dir_w},   // 0x2e
    {M_and, A_dir_l},   // 0x2f
    {M_bmi, A_rel8},    // 0x30
    {M_and, A_ind_by},  // 0x31
    {M_and, A_ind_b},   // 0x32
    {M_and, A_stk_y},   // 0x33
    {M_bit, A_dir_bx},  // 0x34
    {M_and, A_dir_bx},  // 0x35
    {M_rol, A_dir_bx},  // 0x36
    {M_and, A_lng_by},  // 0x37
    {M_sec, A_imp},     // 0x38
    {M_and, A_dir_wy},  // 0x39
    {M_dec, A_acc},     // 0x3a
    {M_tsc, A_imp},     // 0x3b
    {M_bit, A_dir_wx},  // 0x3c
    {M_and, A_dir_wx},  // 0x3d
    {M_rol, A_dir_wx},  // 0x3e
    {M_and, A_dir_lx},  // 0x3f
    {M_rti, A_imp},     // 0x40
    {M_eor, A_ind_bx},  // 0x41
    {M_wdm, A_imm_b},   // 0x42
    {M_eor, A_stk},     // 0x43
    {M_mvp, A_mov},     // 0x44
    {M_eor, A_dir_b},   // 0x45
    {M_lsr, A_dir_b},   // 0x46
    {M_eor, A_lng_b},   // 0x47
    {M_pha, A_imp},     // 0x48
    {M_eor, A_imm_fm},  // 0x49
    {M_lsr, A_acc},     // 0x4a
    {M_phk, A_imp},     // 0x4b
    {M_jmp, A_dir_w},   // 0x4c
    {M_eor, A_dir_w},   // 0x4d
    {M_lsr, A_dir_w},   // 0x4e
    {M_eor, A_dir_l},   // 0x4f
    {M_bvc, A_rel8},    // 0x50
    {M_eor, A_ind_by},  // 0x51
    {M_eor, A_ind_b},   // 0x52
    {M_eor, A_stk_y},   // 0x53
    {M_mvn, A_mov},     // 0x54
    {M_eor, A_dir_bx},  // 0x55
    {M_lsr, A_dir_bx},  // 0x56
    {M_eor, A_lng_by},  // 0x57
    {M_cli, A_imp},     // 0x58
    {M_eor, A_dir_wy},  // 0x59
    {M_phy, A_imp},     // 0x5a
    {M_tcd, A_imp},     // 0x5b
    {M_jmp, A_dir_l},   // 0x5c
    {M_eor, A_dir_wx},  // 0x5d
    {M_lsr, A_dir_wx},  // 0x5e
    {M_eor, A_dir_lx},  // 0x5f
    {M_rts, A_imp},     // 0x60
    {M_adc, A_ind_bx},  // 0x61
    {M_per, A_rel16},   // 0x62
    {M_adc, A_stk},     // 0x63
    {M_stz, A_dir_b},   // 0x64
    {M_adc, A_dir_b},   // 0x65
    {M_ror, A_dir_b},   // 0x66
    {M_adc, A_lng_b},   // 0x67
    {M_pla, A_imp},     // 0x68
    {M_adc, A_imm_fm},  // 0x69
    {M_ror, A_acc},     // 0x6a
    {M_rtl, A_imp},     // 0x6b
    {M_jmp, A_ind_w},   // 0x6c
    {M_adc, A_dir_w},   // 0x6d
    {M_ror, A_dir_w},   // 0x6e
    {M_adc, A_dir_l},   // 0x6f
    {M_bvs, A_rel8},    // 0x70
    {M_adc, A_ind_by},  // 0x71
    {M_adc, A_ind_b},   // 0x72
    {M_adc, A_stk_y},   // 0x73
    {M_stz, A_dir_bx},  // 0x74
    {M_adc, A_dir_bx},  // 0x75
    {M_ror, A_dir_bx},  // 0x76
    {M_adc, A_lng_by},  // 0x77
    {M_sei, A_imp},     // 0x78
    {M_adc, A_dir_wy},  // 0x79
    {M_ply, A_imp},     // 0x7a
    {M_tdc, A_imp},     // 0x7b
    {M_jmp, A_ind_wx},  // 0x7c
    {M_adc, A_dir_wx},  // 0x7d
    {M_ror, A_dir_wx},  // 0x7e
    {M_adc, A_dir_lx},  // 0x7f
    {M_bra, A_rel8},    // 0x80
    {M_sta, A_ind_bx},  // 0x81
    {M_brl, A_rel16},   // 0x82
    {M_sta, A_stk},     // 0x83
    {M_sty, A_dir_b},   // 0x84
    {M_sta, A_dir_b},   // 0x85
    {M_stx, A_dir_b},   // 0x86
    {M_sta, A_lng_b},   // 0x87
    {M_dey, A_imp},     // 0x88
    {M_bit, A_imm_fm},  // 0x89
    {M_txa, A_imp},     // 0x8a
    {M_phb, A_imp},     // 0x8b
    {M_sty, A_dir_w},   // 0x8c
    {M_sta, A_dir_w},   // 0x8d
    {M_stx, A_dir_w},   // 0x8e
    {M_sta, A_dir_l},   // 0x8f
    {M_bcc, A_rel8},    // 0x90
    {M_sta, A_ind_by},  // 0x91
    {M_sta, A_ind_b},   // 0x92
    {M_sta, A_stk_y},   // 0x93
    {M_sty, A_dir_bx},  // 0x94
    {M_sta, A_dir_bx},  // 0x95
    {M_stx, A_dir_by},  // 0x96
    {M_sta, A_lng_by},  // 0x97
    {M_tya, A_imp},     // 0x98
    {M_sta, A_dir_wy},  // 0x99
    {M_txs, A_imp},     // 0x9a
    {M_txy, A_imp},     // 0x9b
    {M_stz, A_dir_w},   // 0x9c
    {M_sta, A_dir_wx},  // 0x9d
    {M_stz, A_dir_wx},  // 0x9e
    {M_sta, A_dir_lx},  // 0x9f
    {M_ldy, A_imm_fx},  // 0xa0
    {M_lda, A_ind_bx},  // 0xa1
    {M_ldx, A_imm_fx},  // 0xa2
    {M_lda, A_stk},     // 0xa3
    {M_ldy, A_dir_b},   // 0xa4
    {M_lda, A_dir_b},   // 0xa5
    {M_ldx, A_dir_b},   // 0xa6
    {M_lda, A_lng_b},   // 0xa7
    {M_tay, A_imp},     // 0xa8
    {M_lda, A_imm_fm},  // 0xa9
    {M_tax, A_imp},     // 0xaa
    {M_plb, A_imp},     // 0xab
    {M_ldy, A_dir_w},   // 0xac
    {M_lda, A_dir_w},   // 0xad
    {M_ldx, A_dir_w},   // 0xae
    {M_lda, A_dir_l},   // 0xaf
    {M_bcs, A_rel8},    // 0xb0
    {M_lda, A_ind_by},  // 0xb1
    {M_lda, A_ind_b},   // 0xb2
    {M_lda, A_stk_y},   // 0xb3
    {M_ldy, A_dir_bx},  // 0xb4
    {M_lda, A_dir_bx},  // 0xb5
    {M_ldx, A_dir_by},  // 0xb6
    {M_lda, A_lng_by},  // 0xb7
    {M_clv, A_imp},     // 0xb8
    {M_lda, A_dir_wy},  // 0xb9
    {M_tsx, A_imp},     // 0xba
    {M_tyx, A_imp},     // 0xbb
    {M_ldy, A_dir_wx},  // 0xbc
    {M_lda, A_dir_wx},  // 0xbd
    {M_ldx, A_dir_wy},  // 0xbe
    {M_lda, A_dir_lx},  // 0xbf
    {M_cpy, A_imm_fx},  // 0xc0
    {M_cmp, A_ind_bx},  // 0xc1
    {M_rep, A_imm_b},   // 0xc2
    {M_cmp, A_stk},     // 0xc3
    {M_cpy, A_dir_b},   // 0xc4
    {M_cmp, A_dir_b},   // 0xc5
    {M_dec, A_dir_b},   // 0xc6
    {M_cmp, A_lng_b},   // 0xc7
    {M_iny, A_imp},     // 0xc8
    {M_cmp, A_imm_fm},  // 0xc9
    {M_dex, A_imp},     // 0xca
    {M_wai, A_imp},     // 0xcb
    {M_cpy, A_dir_w},   // 0xcc
    {M_cmp, A_dir_w},   // 0xcd
    {M_dec, A_dir_w},   // 0xce
    {M_cmp, A_dir_l},   // 0xcf
    {M_bne, A_rel8},    // 0xd0
    {M_cmp, A_ind_by},  // 0xd1
    {M_cmp, A_ind_b},   // 0xd2
    {M_cmp, A_stk_y},   // 0xd3
    {M_pei, A_dir_b},   // 0xd4
    {M_cmp, A_dir_bx},  // 0xd5
    {M_dec, A_dir_bx},  // 0xd6
    {M_cmp, A_lng_by},  // 0xd7
    {M_cld, A_imp},     // 0xd8
    {M_cmp, A_dir_wy},  // 0xd9
    {M_phx, A_imp},     // 0xda
    {M_stp, A_imp},     // 0xdb
    {M_jmp, A_lng_w},   // 0xdc
    {M_cmp, A_dir_wx},  // 0xdd
    {M_dec, A_dir_wx},  // 0xde
    {M_cmp, A_dir_lx},  // 0xdf
    {M_cpx, A_imm_fx},  // 0xe0
    {M_sbc, A_ind_bx},  // 0xe1
    {M_sep, A_imm_b},   // 0xe2
    {M_sbc, A_stk},     // 0xe3
    {M_cpx, A_dir_b},   // 0xe4
    {M_sbc, A_dir_b},   // 0xe5
    {M_inc, A_dir_b},   // 0xe6
    {M_sbc, A_lng_b},   // 0xe7
    {M_inx, A_imp},     // 0xe8
    {M_sbc, A_imm_fm},  // 0xe9
    {M_nop, A_imp},     // 0xea
    {M_xba, A_imp},     // 0xeb
    {M_cpx, A_dir_w},   // 0xec
    {M_sbc, A_dir_w},   // 0xed
    {M_inc, A_dir_w},   // 0xee
    {M_sbc, A_dir_l},   // 0xef
    {M_beq, A_rel8},    // 0xf0
    {M_sbc, A_ind_by},  // 0xf1
    {M_sbc, A_ind_b},   // 0xf2
    {M_sbc, A_stk_y},   // 0xf3
    {M_pea, A_imm_w},   // 0xf4
    {M_sbc, A_dir_bx},  // 0xf5
    {M_inc, A_dir_bx},  // 0xf6
    {M_sbc, A_lng_by},  // 0xf7
    {M_sed, A_imp},     // 0xf8
    {M_sbc, A_dir_wy},  // 0xf9
    {M_plx, A_imp},     // 0xfa
    {M_xce, A_imp},     // 0xfb
    {M_jsr, A_ind_wx},  // 0xfc
    {M_sbc, A_dir_wx},  // 0xfd
    {M_inc, A_dir_wx},  // 0xfe
    {M_sbc, A_dir_lx},  // 0xff
};

absl::flat_hash_map<DecodeMapEntry, uint8_t> MakeReverseOpcodeMap() {
  absl::flat_hash_map<DecodeMapEntry, uint8_t> reverse;
  for (int i = 0; i < 256; ++i) {
    DecodeMapEntry entry = decode_map[i];
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

const absl::flat_hash_map<DecodeMapEntry, uint8_t>& ReverseOpcodeMap() {
  static auto* map =
      new absl::flat_hash_map<DecodeMapEntry, uint8_t>(MakeReverseOpcodeMap());
  return *map;
}

}  // namespace

std::pair<Mnemonic, AddressingMode> DecodeOpcode(uint8_t opcode) {
  return decode_map[opcode];
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
  DecodeMapEntry e(m, A_imm_fm);
  return ReverseOpcodeMap().contains(e) || m == PM_add || m == PM_sub;
}

bool ImmediateArgumentUsesXBit(Mnemonic m) {
  DecodeMapEntry e(m, A_imm_fx);
  return ReverseOpcodeMap().contains(e);
}

bool TakesOffsetArgument(Mnemonic m) {
  DecodeMapEntry e8(m, A_rel8);
  DecodeMapEntry e16(m, A_rel16);
  return ReverseOpcodeMap().contains(e8) || ReverseOpcodeMap().contains(e16);
}

bool TakesLongOffsetArgument(Mnemonic m) {
  DecodeMapEntry e16(m, A_rel16);
  return ReverseOpcodeMap().contains(e16);
}

bool IsLegalCombination(Mnemonic m, AddressingMode a) {
  auto opcode_it = ReverseOpcodeMap().find(std::make_pair(m, a));
  return opcode_it != ReverseOpcodeMap().end();
}

}  // namespace nsasm
