#include "nsasm/decode.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

NullLookupContext lookup_context;

TEST(Decode, mode_dependent_instructions) {
  for (int i = 0; i < 256; ++i) {
    uint8_t opcode = i;
    Mnemonic mnemonic;
    AddressingMode addressing_mode;
    std::tie(mnemonic, addressing_mode) = DecodeOpcode(i);
    if (addressing_mode != A_imm_fm && addressing_mode != A_imm_fx) {
      continue;
    }

    FlagState unknown_flags;
    FlagState eight_bit_flags;
    FlagState sixteen_bit_flags;
    if (addressing_mode == A_imm_fm) {
      eight_bit_flags.SetMBit(B_on);
      sixteen_bit_flags.SetMBit(B_off);
    } else if (addressing_mode == A_imm_fx) {
      eight_bit_flags.SetXBit(B_on);
      sixteen_bit_flags.SetXBit(B_off);
    }

    // Not enough bytes
    std::vector<uint8_t> small_data = {opcode};
    EXPECT_FALSE(Decode(small_data, unknown_flags).ok());
    EXPECT_FALSE(Decode(small_data, eight_bit_flags).ok());
    EXPECT_FALSE(Decode(small_data, sixteen_bit_flags).ok());

    std::vector<uint8_t> data = {opcode, 0x21, 0x43, 0x65};
    EXPECT_FALSE(Decode(data, unknown_flags).ok());

    auto byte_ins = Decode(data, eight_bit_flags);
    NSASM_ASSERT_OK(byte_ins);
    EXPECT_EQ(byte_ins->mnemonic, mnemonic);
    EXPECT_EQ(byte_ins->addressing_mode, A_imm_b);
    EXPECT_EQ(byte_ins->SerializedSize(), 2);
    EXPECT_EQ(byte_ins->arg1.Evaluate(lookup_context), 0x21);

    auto word_ins = Decode(data, sixteen_bit_flags);
    NSASM_ASSERT_OK(word_ins);
    EXPECT_EQ(word_ins->mnemonic, mnemonic);
    EXPECT_EQ(word_ins->addressing_mode, A_imm_w);
    EXPECT_EQ(word_ins->SerializedSize(), 3);
    EXPECT_EQ(word_ins->arg1.Evaluate(lookup_context), 0x4321);
  }
}

TEST(Decode, mode_independent_instructions) {
  for (int i = 0; i < 256; ++i) {
    uint8_t opcode = i;
    Mnemonic mnemonic;
    AddressingMode addressing_mode;
    std::tie(mnemonic, addressing_mode) = DecodeOpcode(i);
    if (addressing_mode == A_imm_fm || addressing_mode == A_imm_fx) {
      continue;
    }

    std::vector<uint8_t> data = {opcode, 0x21, 0x43, 0x65, 0x87};

    // Check that we error on too few bytes, but accept the correct size
    EXPECT_FALSE(Decode(absl::MakeSpan(data).subspan(
                            0, InstructionLength(addressing_mode) - 1),
                        FlagState())
                     .ok());
    EXPECT_TRUE(Decode(absl::MakeSpan(data).subspan(
                           0, InstructionLength(addressing_mode)),
                       FlagState())
                    .ok());

    auto decoded = Decode(data, FlagState());
    NSASM_ASSERT_OK(decoded);
    EXPECT_EQ(decoded->mnemonic, mnemonic);
    EXPECT_EQ(decoded->addressing_mode, addressing_mode);
    EXPECT_EQ(decoded->SerializedSize(), InstructionLength(addressing_mode));

    // Handle each addressing mode based solely on the size of its arguments,
    // since within each class of these, the decoding is identical.
    if (addressing_mode == A_imp || addressing_mode == A_acc) {
      EXPECT_FALSE(decoded->arg2);
      EXPECT_FALSE(decoded->arg1);
    } else if (addressing_mode == A_imm_b || addressing_mode == A_dir_b ||
               addressing_mode == A_dir_bx || addressing_mode == A_dir_by ||
               addressing_mode == A_ind_b || addressing_mode == A_ind_bx ||
               addressing_mode == A_ind_by || addressing_mode == A_lng_b ||
               addressing_mode == A_lng_by || addressing_mode == A_stk ||
               addressing_mode == A_stk_y || addressing_mode == A_rel8) {
      EXPECT_FALSE(decoded->arg2);
      ASSERT_TRUE(decoded->arg1);
      EXPECT_EQ(decoded->arg1.Evaluate(lookup_context), 0x21);
    } else if (addressing_mode == A_imm_w || addressing_mode == A_dir_w ||
               addressing_mode == A_dir_wx || addressing_mode == A_dir_wy ||
               addressing_mode == A_ind_w || addressing_mode == A_ind_wx ||
               addressing_mode == A_lng_w || addressing_mode == A_rel16) {
      EXPECT_FALSE(decoded->arg2);
      ASSERT_TRUE(decoded->arg1);
      EXPECT_EQ(decoded->arg1.Evaluate(lookup_context), 0x4321);
    } else if (addressing_mode == A_dir_l || addressing_mode == A_dir_lx) {
      EXPECT_FALSE(decoded->arg2);
      ASSERT_TRUE(decoded->arg1);
      EXPECT_EQ(decoded->arg1.Evaluate(lookup_context), 0x654321);
    } else if (addressing_mode == A_mov) {
      ASSERT_TRUE(decoded->arg1);
      ASSERT_TRUE(decoded->arg2);
      EXPECT_EQ(decoded->arg1.Evaluate(lookup_context), 0x43);
      EXPECT_EQ(decoded->arg2.Evaluate(lookup_context), 0x21);
    }
  }
}

}  // namespace nsasm
