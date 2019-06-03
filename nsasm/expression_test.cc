#include "nsasm/expression.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "nsasm/parse.h"

namespace nsasm {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {
ExpressionOrNull Ex(absl::string_view sv) {
  auto expr = ParseExpression(sv);
  NSASM_EXPECT_OK(expr);
  return expr.ok() ? *expr : ExpressionOrNull();
}

int Eval(absl::string_view sv) {
  auto val = Ex(sv).Evaluate(NullLookupContext());
  NSASM_EXPECT_OK(val);
  return val.ok() ? *val : -999;
}
}  // namespace

TEST(Expression, order_of_operations) {
  EXPECT_EQ(Ex("1+2*3").ToString(), "op+(1, op*(2, 3))");
  EXPECT_EQ(Ex("1+(2*3)").ToString(), "op+(1, op*(2, 3))");
  EXPECT_EQ(Ex("(1+2)*3").ToString(), "op*(op+(1, 2), 3)");
  EXPECT_EQ(Ex("1*2+3").ToString(), "op+(op*(1, 2), 3)");
  EXPECT_EQ(Ex("5-2-1").ToString(), "op-(op-(5, 2), 1)");

  EXPECT_EQ(Eval("1+2*3"), 7);
  EXPECT_EQ(Eval("1+(2*3)"), 7);
  EXPECT_EQ(Eval("(1+2)*3"), 9);
  EXPECT_EQ(Eval("1*2+3"), 5);
  EXPECT_EQ(Eval("5-2-1"), 2);

  EXPECT_EQ(Ex("-1-2").ToString(), "op-(op-(1), 2)");
  EXPECT_EQ(Ex("-(1-2)").ToString(), "op-(op-(1, 2))");
  EXPECT_EQ(Ex("1--2").ToString(), "op-(1, op-(2))");

  EXPECT_EQ(Eval("-1-2"), -3);
  EXPECT_EQ(Eval("-(1-2)"), 1);
  EXPECT_EQ(Eval("1--2"), 3);

  EXPECT_EQ(Ex("foo+bar").ToString(), "op+(foo, bar)");
}

TEST(Expression, byte_expressions) {
  EXPECT_EQ(Ex("<$123456").ToString(), "op<($123456)");
  EXPECT_EQ(Ex(">$123456").ToString(), "op>($123456)");
  EXPECT_EQ(Ex("^$123456").ToString(), "op^($123456)");

  EXPECT_EQ(Eval("<$123456"), 0x56);
  EXPECT_EQ(Eval(">$123456"), 0x34);
  EXPECT_EQ(Eval("^$123456"), 0x12);
}

TEST(Expression, literal_type) {
  EXPECT_EQ(Ex("0").Type(), T_unknown);
  EXPECT_EQ(Ex("00000000").Type(), T_unknown);
  EXPECT_EQ(Ex("$00").Type(), T_byte);
  EXPECT_EQ(Ex("$0000").Type(), T_word);
  EXPECT_EQ(Ex("$000000").Type(), T_long);
}

TEST(Expression, identifiers) {
  EXPECT_EQ(Ex("foo").ToString(), "foo");
  EXPECT_EQ(Ex("foo::bar").ToString(), "foo::bar");
  EXPECT_EQ(Ex("@foo").ToString(), "@foo");
  EXPECT_EQ(Ex("@foo::bar").ToString(), "@foo::bar");

  EXPECT_EQ(Ex("foo").Type(), T_word);
  EXPECT_EQ(Ex("foo::bar").Type(), T_word);
  EXPECT_EQ(Ex("@foo").Type(), T_long);
  EXPECT_EQ(Ex("@foo::bar").Type(), T_long);

  EXPECT_EQ(Ex("foo + 1").Type(), T_word);
  EXPECT_EQ(Ex("foo::bar + 1").Type(), T_word);
  EXPECT_EQ(Ex("@foo + 1").Type(), T_long);
  EXPECT_EQ(Ex("@foo::bar + 1").Type(), T_long);
}

TEST(Expression, namespaces) {
  EXPECT_THAT(Ex("foo::bar + foo::baz").ModuleNamesReferenced(),
              ElementsAre("foo"));
  EXPECT_THAT(Ex("foo::bar + baz::blat").ModuleNamesReferenced(),
              ElementsAre("baz", "foo"));
  EXPECT_THAT(Ex("foo + bar + baz").ModuleNamesReferenced(),
              IsEmpty());
}

}  // namespace nsasm
