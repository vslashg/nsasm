#include "nsasm/token.h"

#include <vector>

#include "gtest/gtest.h"

namespace nsasm {

template <typename... T>
std::vector<Token> TokenVector(const T&... arg) {
  return {Token(arg, Location())..., Token(EndOfLine(), Location())};
}

TEST(Token, names) {
  {
    // Test that register names are reserved (aren't scanned as identifiers)
    auto x = Tokenize("a b c x y z", Location());
    NSASM_ASSERT_OK(x);
    EXPECT_EQ(*x, TokenVector('A', "b", "c", 'X', 'Y', "z"));
  }
  {
    // Similarly for mnemonics and pseudomnemonics
    auto x = Tokenize("add mul sbc div", Location());
    NSASM_ASSERT_OK(x);
    EXPECT_EQ(*x, TokenVector(PM_add, "mul", M_sbc, "div"));
  }
  {
    // And for keywords
    auto x = Tokenize("import/export yiELds IMPORT/EXPORT", Location());
    NSASM_ASSERT_OK(x);
    EXPECT_EQ(*x, TokenVector("import", '/', P_export, P_yields, "IMPORT", '/',
                              P_export));
  }
  {
    // Test that numbers are read correctly and type is inferred from value
    auto x = Tokenize("$12 $0012 $000012 0x12 0x0012 0x000012 12 000012",
                      Location());
    NSASM_ASSERT_OK(x);
    // six hex, two decimal
    EXPECT_EQ(*x, TokenVector(0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 12, 12));
    if (x->size() == 8) {
      EXPECT_EQ((*x)[0].Type(), T_byte);
      EXPECT_EQ((*x)[1].Type(), T_word);
      EXPECT_EQ((*x)[2].Type(), T_long);
      EXPECT_EQ((*x)[3].Type(), T_byte);
      EXPECT_EQ((*x)[4].Type(), T_word);
      EXPECT_EQ((*x)[5].Type(), T_long);
      EXPECT_EQ((*x)[6].Type(), T_unknown);
      EXPECT_EQ((*x)[7].Type(), T_unknown);
    }
  }
  {
    // Sanity check that something that looks like assembly can be tokenized
    auto x = Tokenize("label1 ADC ($1234,X)", Location());
    NSASM_ASSERT_OK(x);
    EXPECT_EQ(*x, TokenVector("label1", M_adc, '(', 0x1234, ',', 'X', ')'));
  }
  {
    // Sanity check that something that looks like assembly can be tokenized
    auto x = Tokenize("label1 JSL @module::func", Location());
    NSASM_ASSERT_OK(x);
    EXPECT_EQ(*x, TokenVector("label1", M_jsl, '@', "module", P_scope, "func"));
  }
  {
    // Sanity check that directives can be tokenized
    auto x = Tokenize("label1 .DB $12, $34, foo", Location());
    NSASM_ASSERT_OK(x);
    EXPECT_EQ(*x, TokenVector("label1", D_db, 0x12, ',', 0x34, ',', "foo"));
  }
}

TEST(Token, convenience_equality_operator) {
  EXPECT_EQ(Token('@', Location()), '@');
  EXPECT_EQ(Token(P_scope, Location()), P_scope);
  EXPECT_EQ(Token(12345, Location()), 12345);
  EXPECT_EQ(Token("abcde", Location()), "abcde");
  EXPECT_EQ(Token(M_adc, Location()), M_adc);
}

}  // namespace nsasm
