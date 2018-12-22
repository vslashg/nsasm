#include "nsasm/mnemonic.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"

namespace nsasm {
namespace {

constexpr absl::string_view mnemonic_names[] = {
    "ADC", "AND", "ASL", "BIT", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DEC",
    "DEX", "DEY", "EOR", "INC", "INX", "INY", "LDA", "LDX", "LDY", "LSR", "MVN",
    "MVP", "NOP", "ORA", "PEA", "PEI", "PER", "PHB", "PHD", "PHK", "PLB", "PLD",
    "ROL", "ROR", "SBC", "SED", "SEI", "STA", "STP", "STX", "STY", "STZ", "TAX",
    "TAY", "TCD", "TCS", "TDC", "TRB", "TSB", "TSC", "TSX", "TXA", "TXS", "TXY",
    "TYA", "TYX", "WAI", "WDM", "XBA", "BCC", "BCS", "BEQ", "BMI", "BNE", "BPL",
    "BRA", "BRK", "BRL", "BVC", "BVS", "COP", "JMP", "JSL", "JSR", "RTI", "RTL",
    "RTS", "CLC", "PHP", "PLP", "REP", "SEC", "SEP", "XCE",
};

}  // namespace

absl::string_view ToString(Mnemonic m) {
  if (m < M_adc || m > M_xce) {
    return "";
  }
  return mnemonic_names[m];
}

absl::optional<Mnemonic> ToMnemonic(std::string s) {
  static absl::flat_hash_map<absl::string_view, Mnemonic> lookup = {
      {"ADC", M_adc}, {"AND", M_and}, {"ASL", M_asl}, {"BIT", M_bit},
      {"CLD", M_cld}, {"CLI", M_cli}, {"CLV", M_clv}, {"CMP", M_cmp},
      {"CPX", M_cpx}, {"CPY", M_cpy}, {"DEC", M_dec}, {"DEX", M_dex},
      {"DEY", M_dey}, {"EOR", M_eor}, {"INC", M_inc}, {"INX", M_inx},
      {"INY", M_iny}, {"LDA", M_lda}, {"LDX", M_ldx}, {"LDY", M_ldy},
      {"LSR", M_lsr}, {"MVN", M_mvn}, {"MVP", M_mvp}, {"NOP", M_nop},
      {"ORA", M_ora}, {"PEA", M_pea}, {"PEI", M_pei}, {"PER", M_per},
      {"PHB", M_phb}, {"PHD", M_phd}, {"PHK", M_phk}, {"PLB", M_plb},
      {"PLD", M_pld}, {"ROL", M_rol}, {"ROR", M_ror}, {"SBC", M_sbc},
      {"SED", M_sed}, {"SEI", M_sei}, {"STA", M_sta}, {"STP", M_stp},
      {"STX", M_stx}, {"STY", M_sty}, {"STZ", M_stz}, {"TAX", M_tax},
      {"TAY", M_tay}, {"TCD", M_tcd}, {"TCS", M_tcs}, {"TDC", M_tdc},
      {"TRB", M_trb}, {"TSB", M_tsb}, {"TSC", M_tsc}, {"TSX", M_tsx},
      {"TXA", M_txa}, {"TXS", M_txs}, {"TXY", M_txy}, {"TYA", M_tya},
      {"TYX", M_tyx}, {"WAI", M_wai}, {"WDM", M_wdm}, {"XBA", M_xba},
      {"BCC", M_bcc}, {"BCS", M_bcs}, {"BEQ", M_beq}, {"BMI", M_bmi},
      {"BNE", M_bne}, {"BPL", M_bpl}, {"BRA", M_bra}, {"BRK", M_brk},
      {"BRL", M_brl}, {"BVC", M_bvc}, {"BVS", M_bvs}, {"COP", M_cop},
      {"JMP", M_jmp}, {"JSL", M_jsl}, {"JSR", M_jsr}, {"RTI", M_rti},
      {"RTL", M_rtl}, {"RTS", M_rts}, {"CLC", M_clc}, {"PHP", M_php},
      {"PLP", M_plp}, {"REP", M_rep}, {"SEC", M_sec}, {"SEP", M_sep},
      {"XCE", M_xce},
  };
  absl::AsciiStrToUpper(&s);
  auto iter = lookup.find(s);
  if (iter == lookup.end()) {
    return absl::nullopt;
  }
  return iter->second;
}

}  // namespace nsasm
