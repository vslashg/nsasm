#include "nsasm/addressing_mode.h"

#include "gtest/gtest.h"
#include "nsasm/expression.h"
#include "nsasm/flag_state.h"
#include "nsasm/instruction.h"
#include "nsasm/numeric_type.h"
#include "nsasm/opcode_map.h"

namespace nsasm {
namespace {

struct TestCase {
  AddressingMode mode;
  int arg1;
  int arg2;
  const char* expected;
};

constexpr TestCase test_cases[] = {
    {A_imp, 0, 0, ""},
    {A_acc, 0, 0, ""},

    {A_imm_b, 0, 0, " #$00"},
    {A_imm_b, 0x12, 0, " #$12"},
    {A_imm_w, 0, 0, " #$0000"},
    {A_imm_w, 0x12, 0, " #$0012"},
    {A_imm_w, 0x1234, 0, " #$1234"},

    {A_dir_b, 0, 0, " $00"},
    {A_dir_b, 0x12, 0, " $12"},
    {A_dir_w, 0, 0, " $0000"},
    {A_dir_w, 0x12, 0, " $0012"},
    {A_dir_w, 0x1234, 0, " $1234"},
    {A_dir_l, 0, 0, " $000000"},
    {A_dir_l, 0x12, 0, " $000012"},
    {A_dir_l, 0x123456, 0, " $123456"},

    {A_dir_bx, 0, 0, " $00, X"},
    {A_dir_bx, 0x12, 0, " $12, X"},
    {A_dir_by, 0, 0, " $00, Y"},
    {A_dir_by, 0x12, 0, " $12, Y"},
    {A_dir_wx, 0, 0, " $0000, X"},
    {A_dir_wx, 0x12, 0, " $0012, X"},
    {A_dir_wx, 0x1234, 0, " $1234, X"},
    {A_dir_wy, 0, 0, " $0000, Y"},
    {A_dir_wy, 0x12, 0, " $0012, Y"},
    {A_dir_wy, 0x1234, 0, " $1234, Y"},

    {A_ind_b, 0, 0, " ($00)"},
    {A_ind_b, 0x12, 0, " ($12)"},
    {A_ind_w, 0, 0, " ($0000)"},
    {A_ind_w, 0x12, 0, " ($0012)"},
    {A_ind_w, 0x1234, 0, " ($1234)"},

    {A_ind_bx, 0, 0, " ($00, X)"},
    {A_ind_bx, 0x12, 0, " ($12, X)"},
    {A_ind_by, 0, 0, " ($00), Y"},
    {A_ind_by, 0x12, 0, " ($12), Y"},
    {A_ind_wx, 0, 0, " ($0000, X)"},
    {A_ind_wx, 0x12, 0, " ($0012, X)"},
    {A_ind_wx, 0x1234, 0, " ($1234, X)"},

    {A_lng_b, 0, 0, " [$00]"},
    {A_lng_b, 0x12, 0, " [$12]"},
    {A_lng_w, 0, 0, " [$0000]"},
    {A_lng_w, 0x12, 0, " [$0012]"},
    {A_lng_w, 0x1234, 0, " [$1234]"},
    {A_lng_by, 0, 0, " [$00], Y"},
    {A_lng_by, 0x12, 0, " [$12], Y"},

    {A_stk, 0, 0, " $00, S"},
    {A_stk, 0x12, 0, " $12, S"},
    {A_stk_y, 0, 0, " ($00, S), Y"},
    {A_stk_y, 0x12, 0, " ($12, S), Y"},

    {A_mov, 0, 0, " #$00, #$00"},
    {A_mov, 0x12, 0x34, " #$12, #$34"},
};

TEST(AddressingMode, rendering) {
  int index = 0;
  for (const TestCase& test_case : test_cases) {
    SCOPED_TRACE(index++);
    ExpressionOrNull arg1 = absl::make_unique<Literal>(test_case.arg1);
    ExpressionOrNull arg2 = absl::make_unique<Literal>(test_case.arg2);
    EXPECT_EQ(ArgsToString(test_case.mode, arg1, arg2), test_case.expected);
  }
}

TEST(AddressingMode, instruction_size) {
  EXPECT_EQ(InstructionLength(A_imp), 1);
  EXPECT_EQ(InstructionLength(A_acc), 1);
  EXPECT_EQ(InstructionLength(A_imm_b), 2);
  EXPECT_EQ(InstructionLength(A_imm_w), 3);
  EXPECT_EQ(InstructionLength(A_dir_b), 2);
  EXPECT_EQ(InstructionLength(A_dir_w), 3);
  EXPECT_EQ(InstructionLength(A_dir_l), 4);
  EXPECT_EQ(InstructionLength(A_dir_bx), 2);
  EXPECT_EQ(InstructionLength(A_dir_by), 2);
  EXPECT_EQ(InstructionLength(A_dir_wx), 3);
  EXPECT_EQ(InstructionLength(A_dir_wy), 3);
  EXPECT_EQ(InstructionLength(A_dir_lx), 4);
  EXPECT_EQ(InstructionLength(A_ind_b), 2);
  EXPECT_EQ(InstructionLength(A_ind_w), 3);
  EXPECT_EQ(InstructionLength(A_ind_bx), 2);
  EXPECT_EQ(InstructionLength(A_ind_by), 2);
  EXPECT_EQ(InstructionLength(A_ind_wx), 3);
  EXPECT_EQ(InstructionLength(A_lng_b), 2);
  EXPECT_EQ(InstructionLength(A_lng_w), 3);
  EXPECT_EQ(InstructionLength(A_lng_by), 2);
  EXPECT_EQ(InstructionLength(A_stk), 2);
  EXPECT_EQ(InstructionLength(A_stk_y), 2);
  EXPECT_EQ(InstructionLength(A_mov), 3);
  EXPECT_EQ(InstructionLength(A_rel8), 2);
  EXPECT_EQ(InstructionLength(A_rel16), 3);
  EXPECT_EQ(InstructionLength(A_imm_fm), 0);
  EXPECT_EQ(InstructionLength(A_imm_fx), 0);
}

struct SimpleDeductionCase {
  SyntacticAddressingMode sam;
  AddressingMode am[3];
};

TEST(AddressingMode, simple_deduce_mode) {
  AddressingMode none = A_imp;  // sentinel value
  const FlagState dummy_flag_state;

  SimpleDeductionCase test_cases[] = {
      {SA_imm, {A_imm_b, A_imm_w, none}},
      {SA_dir, {A_dir_b, A_dir_w, A_dir_l}},
      {SA_dir_x, {A_dir_bx, A_dir_wx, A_dir_lx}},
      {SA_dir_y, {A_dir_by, A_dir_wy, none}},
      {SA_ind, {A_ind_b, A_ind_w, none}},
      {SA_ind_x, {A_ind_bx, A_ind_wx, none}},
      {SA_ind_y, {A_ind_by, none, none}},
      {SA_lng, {A_lng_b, A_lng_w, none}},
      {SA_lng_y, {A_lng_by, none, none}},
      {SA_stk, {A_stk, none, none}},
      {SA_stk_y, {A_stk_y, none, none}},
  };

  // Test every combination of mnemonic and addressing mode.
  //
  // If the combination of mnemonic and addressing mode is valid, then
  // it should syntactically deduce correctly.  Otherwise, deduction should
  // fail.
  for (Mnemonic m : AllMnemonics()) {
    SCOPED_TRACE(ToString(m));
    for (const auto& test_case : test_cases) {
      SCOPED_TRACE(ToString(test_case.sam));

      if (test_case.sam == SA_imm &&
          (ImmediateArgumentUsesMBit(m) || ImmediateArgumentUsesXBit(m))) {
        // This instruction/addressing mode pair cares about processor status
        // bit. Bail out for now; this case is handled by the
        // deduce_immediate_mode test below.
        continue;
      }

      NumericType numeric_type_list[3] = {T_byte, T_word, T_long};
      for (int i = 0; i < 3; ++i) {
        NumericType numeric_type = numeric_type_list[i];
        AddressingMode addressing_mode = test_case.am[i];
        SCOPED_TRACE(ToString(addressing_mode));

        // Construct an argument of the given type
        Literal arg1(0, numeric_type);
        ExpressionOrNull arg2;
        Instruction instruction = {m, addressing_mode, ExpressionOrNull(arg1),
                                   arg2};
        auto deduced = DeduceMode(m, test_case.sam, arg1, arg2);
        if (deduced.ok()) {
          EXPECT_EQ(*deduced, addressing_mode)
              << "DeduceMode() returned a mode of " << ToString(*deduced)
              << " for mnemonic " << ToString(m) << " where "
              << ToString(addressing_mode) << " was expected";
          EXPECT_TRUE(instruction.CheckConsistency(dummy_flag_state).ok())
              << "DeduceMode() returned a mode of " << ToString(*deduced)
              << " for mnemonic " << ToString(m)
              << ", but this is not a valid combination";
        } else {
          // Failed to deduce.  There are two valid reasons for this:
          //   a) the addressing mode never supports the argument type
          //   b) this addressing mode and type is invalid for the mnemonic.
          if (addressing_mode != none) {
            // not in case (a), check that we are in case (b)
            EXPECT_FALSE(instruction.CheckConsistency(dummy_flag_state).ok())
                << "DeduceMode() did not deduce " << ToString(addressing_mode)
                << " argument for mnemonic " << ToString(m)
                << ", but this combination is valid";
          }
        }
      }
    }
  }
}

TEST(AddressingMode, deduce_no_arg_mode) {
  // Test instructions taking no arguments and/or taking A as an argument.
  const ExpressionOrNull null;
  const FlagState dummy_flag_state;

  for (Mnemonic m : AllMnemonics()) {
    SCOPED_TRACE(ToString(m));
    Instruction implied_instruction = {m, A_imp, null, null};
    Instruction accumulator_instruction = {m, A_acc, null, null};

    if (accumulator_instruction.CheckConsistency(dummy_flag_state).ok()) {
      // For instructions that support accumulator mode (like DEC), we should
      // support syntactic forms `DEC A` and `DEC`.
      auto deduced_acc = DeduceMode(m, SA_acc, null, null);
      auto deduced_imp = DeduceMode(m, SA_imp, null, null);
      EXPECT_TRUE(deduced_acc.ok() && (*deduced_acc == A_acc));
      EXPECT_TRUE(deduced_imp.ok() && (*deduced_imp == A_acc));
    } else if (implied_instruction.CheckConsistency(dummy_flag_state).ok()) {
      // For instructions that take no argumnet (like RTS), we should accept
      // `RTS` but not `RTS A`
      auto deduced_acc = DeduceMode(m, SA_acc, null, null);
      auto deduced_imp = DeduceMode(m, SA_imp, null, null);
      EXPECT_FALSE(deduced_acc.ok());
      EXPECT_TRUE(deduced_imp.ok() && (*deduced_imp == A_imp));
    } else {
      // Other instructions should accept neither form.
      auto deduced_acc = DeduceMode(m, SA_acc, null, null);
      auto deduced_imp = DeduceMode(m, SA_imp, null, null);
      EXPECT_FALSE(deduced_acc.ok());
      EXPECT_FALSE(deduced_imp.ok());
    }
  }
}

TEST(AddressingMode, deduce_immediate_mode) {
  // Test instructions in immediate mode which care about flag bits
  for (Mnemonic m : AllMnemonics()) {
    SCOPED_TRACE(ToString(m));
    if (ImmediateArgumentUsesMBit(m)) {
      // flag state where m bit is known
      FlagState flag_state(B_off, B_off, B_original);
      Literal arg1(0, T_word);
      ExpressionOrNull arg2;

      // Sanity-check ImmediateArgumentUsesMBit()
      Instruction instruction = {m, A_imm_fm, ExpressionOrNull(arg1), arg2};
      EXPECT_TRUE(instruction.CheckConsistency(flag_state).ok());

      // We should deduce this as an instruction that cares about the m bit
      auto deduced = DeduceMode(m, SA_imm, arg1, arg2);
      EXPECT_TRUE(deduced.ok() && *deduced == A_imm_fm);
    }

    if (ImmediateArgumentUsesXBit(m)) {
      // flag state where x bit is known
      FlagState flag_state(B_off, B_original, B_off);
      Literal arg1(0, T_word);
      ExpressionOrNull arg2;

      // Sanity-check ImmediateArgumentUsesXBit()
      Instruction instruction = {m, A_imm_fx, ExpressionOrNull(arg1), arg2};
      EXPECT_TRUE(instruction.CheckConsistency(flag_state).ok());

      // We should deduce this as an instruction that cares about the x bit
      auto deduced = DeduceMode(m, SA_imm, arg1, arg2);
      EXPECT_TRUE(deduced.ok() && *deduced == A_imm_fx);
    }
  }
}

}  // namespace
}  // namespace nsasm