#include "nsasm/parse.h"

#include "gtest/gtest.h"
#include "nsasm/expression.h"
#include "nsasm/opcode_map.h"
#include "nsasm/statement.h"

namespace nsasm {
namespace {

TEST(Parse, round_trip) {
  // round trip every possible instruction through the assembler
  for (int i = 0; i < 256; ++i) {
    nsasm::Instruction ins;
    std::tie(ins.mnemonic, ins.addressing_mode) = nsasm::DecodeOpcode(i);
    if (ins.addressing_mode == A_rel8 || ins.addressing_mode == A_rel16) {
      ins.arg1 =
          absl::make_unique<IdentifierExpression>(FullIdentifier("label"));
    } else if (ins.addressing_mode != A_imp && ins.addressing_mode != A_acc) {
      ins.arg1 = absl::make_unique<Literal>(0, Arg1Type(ins.addressing_mode));
    }
    if (ins.addressing_mode == A_mov) {
      ins.arg2 = absl::make_unique<Literal>(0, T_byte);
    }

    std::string line = absl::StrCat(
        nsasm::ToString(ins.mnemonic),
        nsasm::ArgsToString(ins.addressing_mode, ins.arg1, ins.arg2));
    SCOPED_TRACE(i);
    SCOPED_TRACE(line);
    auto tokens = Tokenize(line, Location());
    NSASM_ASSERT_OK(tokens);
    auto parsed = Parse(*tokens);
    NSASM_ASSERT_OK(parsed);
    ASSERT_EQ(parsed->size(), 1);
    ASSERT_TRUE(absl::holds_alternative<Statement>(parsed->front()));
    auto round_tripped = absl::get<Statement>(parsed->front()).Instruction();
    ASSERT_TRUE(round_tripped);
    EXPECT_EQ(ins.mnemonic, round_tripped->mnemonic);
    EXPECT_EQ(ins.addressing_mode, round_tripped->addressing_mode);
    NullLookupContext context;
    EXPECT_EQ(ins.arg1.Evaluate(context),
              round_tripped->arg1.Evaluate(context));
    EXPECT_EQ(ins.arg2.Evaluate(context),
              round_tripped->arg2.Evaluate(context));
  }
}

}  // namespace
}  // namespace nsasm
