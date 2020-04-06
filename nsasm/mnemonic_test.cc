#include "nsasm/mnemonic.h"

#include "absl/strings/ascii.h"
#include "gtest/gtest.h"

namespace nsasm {
namespace {

// Helper to make conversion tests; intended to be invoked by macro.
void CheckToString(Mnemonic m, std::string str) {
  SCOPED_TRACE(str);
  EXPECT_EQ(ToString(m), str);
  EXPECT_TRUE(ToMnemonic(str).has_value());
  EXPECT_EQ(*ToMnemonic(str), m);

  // upper strings should convert to mnemonics as well
  std::string upper = absl::AsciiStrToUpper(str);
  EXPECT_TRUE(ToMnemonic(upper).has_value());
  EXPECT_EQ(*ToMnemonic(upper), m);
}

void CheckToString(Suffix suf, std::string str) {
  SCOPED_TRACE(str);
  EXPECT_EQ(ToString(suf), str);
  EXPECT_TRUE(ToSuffix(str).has_value());
  EXPECT_EQ(*ToSuffix(str), suf);

  // lowercase strings should convert to mnemonics as well
  std::string upper = absl::AsciiStrToUpper(str);
  EXPECT_TRUE(ToSuffix(upper).has_value());
  EXPECT_EQ(*ToSuffix(upper), suf);
}

#define CHECK_MNEMONIC_TO_STRING(name) CheckToString(M_##name, #name)
#define CHECK_PSEUDO_TO_STRING(name) CheckToString(PM_##name, #name)
#define CHECK_SUFFIX_TO_STRING(name) CheckToString(S_##name, "." #name)

TEST(Mnemonic, string_conversions) {
  CHECK_MNEMONIC_TO_STRING(adc);
  CHECK_MNEMONIC_TO_STRING(and);
  CHECK_MNEMONIC_TO_STRING(asl);
  CHECK_MNEMONIC_TO_STRING(bcc);
  CHECK_MNEMONIC_TO_STRING(bcs);
  CHECK_MNEMONIC_TO_STRING(beq);
  CHECK_MNEMONIC_TO_STRING(bit);
  CHECK_MNEMONIC_TO_STRING(bmi);
  CHECK_MNEMONIC_TO_STRING(bne);
  CHECK_MNEMONIC_TO_STRING(bpl);
  CHECK_MNEMONIC_TO_STRING(bra);
  CHECK_MNEMONIC_TO_STRING(brk);
  CHECK_MNEMONIC_TO_STRING(brl);
  CHECK_MNEMONIC_TO_STRING(bvc);
  CHECK_MNEMONIC_TO_STRING(bvs);
  CHECK_MNEMONIC_TO_STRING(clc);
  CHECK_MNEMONIC_TO_STRING(cld);
  CHECK_MNEMONIC_TO_STRING(cli);
  CHECK_MNEMONIC_TO_STRING(clv);
  CHECK_MNEMONIC_TO_STRING(cmp);
  CHECK_MNEMONIC_TO_STRING(cop);
  CHECK_MNEMONIC_TO_STRING(cpx);
  CHECK_MNEMONIC_TO_STRING(cpy);
  CHECK_MNEMONIC_TO_STRING(dec);
  CHECK_MNEMONIC_TO_STRING(dex);
  CHECK_MNEMONIC_TO_STRING(dey);
  CHECK_MNEMONIC_TO_STRING(eor);
  CHECK_MNEMONIC_TO_STRING(inc);
  CHECK_MNEMONIC_TO_STRING(inx);
  CHECK_MNEMONIC_TO_STRING(iny);
  CHECK_MNEMONIC_TO_STRING(jmp);
  CHECK_MNEMONIC_TO_STRING(jsl);
  CHECK_MNEMONIC_TO_STRING(jsr);
  CHECK_MNEMONIC_TO_STRING(lda);
  CHECK_MNEMONIC_TO_STRING(ldx);
  CHECK_MNEMONIC_TO_STRING(ldy);
  CHECK_MNEMONIC_TO_STRING(lsr);
  CHECK_MNEMONIC_TO_STRING(mvn);
  CHECK_MNEMONIC_TO_STRING(mvp);
  CHECK_MNEMONIC_TO_STRING(nop);
  CHECK_MNEMONIC_TO_STRING(ora);
  CHECK_MNEMONIC_TO_STRING(pea);
  CHECK_MNEMONIC_TO_STRING(pei);
  CHECK_MNEMONIC_TO_STRING(per);
  CHECK_MNEMONIC_TO_STRING(pha);
  CHECK_MNEMONIC_TO_STRING(phb);
  CHECK_MNEMONIC_TO_STRING(phd);
  CHECK_MNEMONIC_TO_STRING(phk);
  CHECK_MNEMONIC_TO_STRING(php);
  CHECK_MNEMONIC_TO_STRING(phx);
  CHECK_MNEMONIC_TO_STRING(phy);
  CHECK_MNEMONIC_TO_STRING(pla);
  CHECK_MNEMONIC_TO_STRING(plb);
  CHECK_MNEMONIC_TO_STRING(pld);
  CHECK_MNEMONIC_TO_STRING(plp);
  CHECK_MNEMONIC_TO_STRING(plx);
  CHECK_MNEMONIC_TO_STRING(ply);
  CHECK_MNEMONIC_TO_STRING(rep);
  CHECK_MNEMONIC_TO_STRING(rol);
  CHECK_MNEMONIC_TO_STRING(ror);
  CHECK_MNEMONIC_TO_STRING(rti);
  CHECK_MNEMONIC_TO_STRING(rtl);
  CHECK_MNEMONIC_TO_STRING(rts);
  CHECK_MNEMONIC_TO_STRING(sbc);
  CHECK_MNEMONIC_TO_STRING(sec);
  CHECK_MNEMONIC_TO_STRING(sed);
  CHECK_MNEMONIC_TO_STRING(sei);
  CHECK_MNEMONIC_TO_STRING(sep);
  CHECK_MNEMONIC_TO_STRING(sta);
  CHECK_MNEMONIC_TO_STRING(stp);
  CHECK_MNEMONIC_TO_STRING(stx);
  CHECK_MNEMONIC_TO_STRING(sty);
  CHECK_MNEMONIC_TO_STRING(stz);
  CHECK_MNEMONIC_TO_STRING(tax);
  CHECK_MNEMONIC_TO_STRING(tay);
  CHECK_MNEMONIC_TO_STRING(tcd);
  CHECK_MNEMONIC_TO_STRING(tcs);
  CHECK_MNEMONIC_TO_STRING(tdc);
  CHECK_MNEMONIC_TO_STRING(trb);
  CHECK_MNEMONIC_TO_STRING(tsb);
  CHECK_MNEMONIC_TO_STRING(tsc);
  CHECK_MNEMONIC_TO_STRING(tsx);
  CHECK_MNEMONIC_TO_STRING(txa);
  CHECK_MNEMONIC_TO_STRING(txs);
  CHECK_MNEMONIC_TO_STRING(txy);
  CHECK_MNEMONIC_TO_STRING(tya);
  CHECK_MNEMONIC_TO_STRING(tyx);
  CHECK_MNEMONIC_TO_STRING(wai);
  CHECK_MNEMONIC_TO_STRING(wdm);
  CHECK_MNEMONIC_TO_STRING(xba);
  CHECK_MNEMONIC_TO_STRING(xce);
  CHECK_PSEUDO_TO_STRING(add);
  CHECK_PSEUDO_TO_STRING(sub);
  CHECK_SUFFIX_TO_STRING(b);
  CHECK_SUFFIX_TO_STRING(w);

  // Invalid strings should not be converted
  EXPECT_FALSE(ToMnemonic("").has_value());
  EXPECT_FALSE(ToMnemonic("hcf").has_value());

  // The no-suffix sentinel should stringize empty
  EXPECT_EQ(ToString(S_none), "");
}

}  // namespace
}  // namespace nsasm
