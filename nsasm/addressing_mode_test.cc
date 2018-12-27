#include "nsasm/addressing_mode.h"

#include "gtest/gtest.h"
#include "nsasm/argument.h"

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
    EXPECT_EQ(ArgsToString(test_case.mode, Argument(test_case.arg1),
                           Argument(test_case.arg2)),
              test_case.expected);
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
  EXPECT_EQ(InstructionLength(A_imm_fm), -1);
  EXPECT_EQ(InstructionLength(A_imm_fx), -1);
}

}  // namespace
}  // namespace nsasm