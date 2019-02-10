#include "nsasm/flag_state.h"

#include "gtest/gtest.h"

namespace nsasm {

namespace {

struct NameTestCase {
  std::string name;
  BitState e_bit;
  BitState m_bit;
  BitState x_bit;
};

TEST(FlagState, Names) {
  const NameTestCase name_test_cases[] = {
      {"unk", B_unknown, B_unknown, B_unknown},
      {"emu", B_on, B_on, B_on},
      {"native", B_off, B_original, B_original},
      {"m8x8", B_off, B_on, B_on},
      {"m8x16", B_off, B_on, B_off},
      {"m8", B_off, B_on, B_original},
      {"m16x8", B_off, B_off, B_on},
      {"m16x16", B_off, B_off, B_off},
      {"m16", B_off, B_off, B_original},
      {"x8", B_off, B_original, B_on},
      {"x16", B_off, B_original, B_off},
  };

  for (const NameTestCase& test_case : name_test_cases) {
    SCOPED_TRACE(test_case.name);
    // Check that a state constructed with the given name has the correct bit
    // values, and that it round trips back
    FlagState state_from_name = *FlagState::FromName(test_case.name);
    EXPECT_EQ(state_from_name.EBit(), test_case.e_bit);
    EXPECT_EQ(state_from_name.MBit(), test_case.m_bit);
    EXPECT_EQ(state_from_name.XBit(), test_case.x_bit);
    EXPECT_EQ(state_from_name.ToName(), test_case.name);

    // And check that a state constructed from the given flags has the expected name
    FlagState state_from_bits(test_case.e_bit, test_case.m_bit, test_case.x_bit);
    EXPECT_EQ(state_from_bits.ToName(), test_case.name);
  }

  // The `m` and `x` bits can be known to be `1` even when the `e` bit is unknown.
  // We still call this state "unk" in naming.
  FlagState e_unknown_mx_known(B_unknown, B_on, B_on);
  EXPECT_EQ(e_unknown_mx_known.MBit(), B_on);
  EXPECT_EQ(e_unknown_mx_known.XBit(), B_on);
  EXPECT_EQ(e_unknown_mx_known.ToName(), "unk");
}

}  // namespace
}  // namespace nsasm