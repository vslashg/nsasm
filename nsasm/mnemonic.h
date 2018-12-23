#ifndef NSASM_MNEMONIC_H_
#define NSASM_MNEMONIC_H_

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace nsasm {

enum Mnemonic : int {
  // Inert operations: mnemonics for opcodes that static analysis does not need
  // to understand.
  M_adc,
  M_and,
  M_asl,
  M_bit,
  M_cld,
  M_cli,
  M_clv,
  M_cmp,
  M_cpx,
  M_cpy,
  M_dec,
  M_dex,
  M_dey,
  M_eor,
  M_inc,
  M_inx,
  M_iny,
  M_lda,
  M_ldx,
  M_ldy,
  M_lsr,
  M_mvn,
  M_mvp,
  M_nop,
  M_ora,
  M_pea,
  M_pei,
  M_per,
  M_pha,
  M_phb,
  M_phd,
  M_phk,
  M_phx,
  M_phy,
  M_pla,
  M_plb,
  M_pld,
  M_plx,
  M_ply,
  M_rol,
  M_ror,
  M_sbc,
  M_sed,
  M_sei,
  M_sta,
  M_stp,
  M_stx,
  M_sty,
  M_stz,
  M_tax,
  M_tay,
  M_tcd,
  M_tcs,
  M_tdc,
  M_trb,
  M_tsb,
  M_tsc,
  M_tsx,
  M_txa,
  M_txs,
  M_txy,
  M_tya,
  M_tyx,
  M_wai,
  M_wdm,
  M_xba,

  // Flow control operations: conditional and unconditional jumps, subroutine
  // calls, etc.
  M_bcc,
  M_bcs,
  M_beq,
  M_bmi,
  M_bne,
  M_bpl,
  M_bra,
  M_brk,
  M_brl,
  M_bvc,
  M_bvs,
  M_cop,
  M_jmp,
  M_jsl,
  M_jsr,
  M_rti,
  M_rtl,
  M_rts,

  // Status operations: operations that can change the x and m status bits, as
  // well as put the processor into native or emulation mode.  (CLC and SEC do
  // not affect processor state directly, but are usually paired with XCE.)
  M_clc,
  M_php,
  M_plp,
  M_rep,
  M_sec,
  M_sep,
  M_xce,
};

// Conversions between Mnemonic values, and the matching strings.
absl::string_view ToString(Mnemonic m);
absl::optional<Mnemonic> ToMnemonic(std::string s);

}  // namespace nsasm

#endif  // NSASM_MNEMONIC_H_
