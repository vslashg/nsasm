#include "nsasm/expression.h"

#include "gtest/gtest.h"
#include "nsasm/parse.h"

namespace nsasm {

namespace {
ExpressionOrNull Ex(absl::string_view sv) {
  auto expr = ParseExpression(sv);
  NSASM_EXPECT_OK(expr);
  return expr.ok() ? *expr : ExpressionOrNull();
}
}

TEST(Expression, order_of_operations) {
  EXPECT_EQ(Ex("1+2*3").ToString(), "op+(1, op*(2, 3))");
  EXPECT_EQ(Ex("1+(2*3)").ToString(), "op+(1, op*(2, 3))");
  EXPECT_EQ(Ex("(1+2)*3").ToString(), "op*(op+(1, 2), 3)");
  EXPECT_EQ(Ex("1*2+3").ToString(), "op+(op*(1, 2), 3)");
  EXPECT_EQ(Ex("5-2-1").ToString(), "op-(op-(5, 2), 1)");

  EXPECT_EQ(Ex("-1-2").ToString(), "op-(op-(1), 2)");
  EXPECT_EQ(Ex("-(1-2)").ToString(), "op-(op-(1, 2))");
  EXPECT_EQ(Ex("1--2").ToString(), "op-(1, op-(2))");

  EXPECT_EQ(Ex("foo+bar").ToString(), "op+(foo, bar)");
}

}  // namespace nsasm
