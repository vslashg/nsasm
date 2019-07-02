#include "nsasm/mnemonic.h"

#include "absl/strings/ascii.h"
#include "gtest/gtest.h"

namespace nsasm {
namespace {

// Helper to make conversion tests; intended to be invoked by macro.
void CheckToString(Mnemonic m, std::string s) {
  SCOPED_TRACE(s);
  std::string lower = s;
  absl::AsciiStrToLower(&lower);
  EXPECT_EQ(ToString(m), lower);
  EXPECT_TRUE(ToMnemonic(lower).has_value());
  EXPECT_EQ(*ToMnemonic(lower), m);

  // lowercase strings should convert to mnemonics as well
  EXPECT_TRUE(ToMnemonic(s).has_value());
  EXPECT_EQ(*ToMnemonic(s), m);
}

#define CHECK_TO_STRING(name) CheckToString(M_##name, #name)
#define CHECK_PSEUDO_TO_STRING(name) CheckToString(PM_##name, #name)

TEST(Mnemonic, string_conversions) {
  CHECK_TO_STRING(adc);
  CHECK_TO_STRING(and);
  CHECK_TO_STRING(asl);
  CHECK_TO_STRING(bcc);
  CHECK_TO_STRING(bcs);
  CHECK_TO_STRING(beq);
  CHECK_TO_STRING(bit);
  CHECK_TO_STRING(bmi);
  CHECK_TO_STRING(bne);
  CHECK_TO_STRING(bpl);
  CHECK_TO_STRING(bra);
  CHECK_TO_STRING(brk);
  CHECK_TO_STRING(brl);
  CHECK_TO_STRING(bvc);
  CHECK_TO_STRING(bvs);
  CHECK_TO_STRING(clc);
  CHECK_TO_STRING(cld);
  CHECK_TO_STRING(cli);
  CHECK_TO_STRING(clv);
  CHECK_TO_STRING(cmp);
  CHECK_TO_STRING(cop);
  CHECK_TO_STRING(cpx);
  CHECK_TO_STRING(cpy);
  CHECK_TO_STRING(dec);
  CHECK_TO_STRING(dex);
  CHECK_TO_STRING(dey);
  CHECK_TO_STRING(eor);
  CHECK_TO_STRING(inc);
  CHECK_TO_STRING(inx);
  CHECK_TO_STRING(iny);
  CHECK_TO_STRING(jmp);
  CHECK_TO_STRING(jsl);
  CHECK_TO_STRING(jsr);
  CHECK_TO_STRING(lda);
  CHECK_TO_STRING(ldx);
  CHECK_TO_STRING(ldy);
  CHECK_TO_STRING(lsr);
  CHECK_TO_STRING(mvn);
  CHECK_TO_STRING(mvp);
  CHECK_TO_STRING(nop);
  CHECK_TO_STRING(ora);
  CHECK_TO_STRING(pea);
  CHECK_TO_STRING(pei);
  CHECK_TO_STRING(per);
  CHECK_TO_STRING(pha);
  CHECK_TO_STRING(phb);
  CHECK_TO_STRING(phd);
  CHECK_TO_STRING(phk);
  CHECK_TO_STRING(php);
  CHECK_TO_STRING(phx);
  CHECK_TO_STRING(phy);
  CHECK_TO_STRING(pla);
  CHECK_TO_STRING(plb);
  CHECK_TO_STRING(pld);
  CHECK_TO_STRING(plp);
  CHECK_TO_STRING(plx);
  CHECK_TO_STRING(ply);
  CHECK_TO_STRING(rep);
  CHECK_TO_STRING(rol);
  CHECK_TO_STRING(ror);
  CHECK_TO_STRING(rti);
  CHECK_TO_STRING(rtl);
  CHECK_TO_STRING(rts);
  CHECK_TO_STRING(sbc);
  CHECK_TO_STRING(sec);
  CHECK_TO_STRING(sed);
  CHECK_TO_STRING(sei);
  CHECK_TO_STRING(sep);
  CHECK_TO_STRING(sta);
  CHECK_TO_STRING(stp);
  CHECK_TO_STRING(stx);
  CHECK_TO_STRING(sty);
  CHECK_TO_STRING(stz);
  CHECK_TO_STRING(tax);
  CHECK_TO_STRING(tay);
  CHECK_TO_STRING(tcd);
  CHECK_TO_STRING(tcs);
  CHECK_TO_STRING(tdc);
  CHECK_TO_STRING(trb);
  CHECK_TO_STRING(tsb);
  CHECK_TO_STRING(tsc);
  CHECK_TO_STRING(tsx);
  CHECK_TO_STRING(txa);
  CHECK_TO_STRING(txs);
  CHECK_TO_STRING(txy);
  CHECK_TO_STRING(tya);
  CHECK_TO_STRING(tyx);
  CHECK_TO_STRING(wai);
  CHECK_TO_STRING(wdm);
  CHECK_TO_STRING(xba);
  CHECK_TO_STRING(xce);
  CHECK_PSEUDO_TO_STRING(add);
  CHECK_PSEUDO_TO_STRING(sub);

  // Invalid strings should not be converted
  EXPECT_FALSE(ToMnemonic("").has_value());
  EXPECT_FALSE(ToMnemonic("hcf").has_value());
}

}  // namespace
}  // namespace nsasm
