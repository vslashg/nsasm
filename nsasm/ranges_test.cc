#include "nsasm/ranges.h"

#include <algorithm>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace nsasm {
namespace {

using testing::ElementsAre;

std::pair<nsasm::Address, nsasm::Address> Ch(int a, int b) {
  return std::make_pair(nsasm::Address(a), nsasm::Address(b));
}
nsasm::Address Ad(int a) { return nsasm::Address(a); }

TEST(DataRange, Contains) {
  // simple check of containment functionality
  DataRange data_range;
  for (nsasm::Address start : {Ad(20), Ad(40), Ad(60)}) {
    EXPECT_TRUE(data_range.ClaimBytes(start, 10));
  }
  DataRange empty;
  for (nsasm::Address i = Ad(0); i < Ad(80); i = i.AddWrapped(1)) {
    SCOPED_TRACE(i);
    // The point is in data_range if the 10s place digit is 2, 4, or 6
    int digit = i.BankAddress() / 10;
    bool expected = (digit == 2 || digit == 4 || digit == 6);
    EXPECT_EQ(data_range.Contains(i), expected);
    EXPECT_FALSE(empty.Contains(i));
  }
}

TEST(DataRange, MergeNeighbors) {
  // check that writing adjacent chunks in any order works.
  {
    std::vector<nsasm::Address> starts = {Ad(20), Ad(30), Ad(40)};
    do {
      DataRange data_range;
      for (nsasm::Address start : starts) {
        EXPECT_TRUE(data_range.ClaimBytes(start, 10));
      }
      EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(20, 50)));
    } while (std::next_permutation(starts.begin(), starts.end()));
  }
  // check that writing non-neighboring chunks in any order works.
  {
    std::vector<nsasm::Address> starts = {Ad(20), Ad(40), Ad(60)};
    do {
      DataRange data_range;
      for (nsasm::Address start : starts) {
        EXPECT_TRUE(data_range.ClaimBytes(start, 10));
      }
      EXPECT_THAT(data_range.Chunks(),
                  ElementsAre(Ch(20, 30), Ch(40, 50), Ch(60, 70)));
    } while (std::next_permutation(starts.begin(), starts.end()));
  }
  // Check that writing slightly-overlapping chunks in any order works.
  {
    std::vector<nsasm::Address> starts = {Ad(20), Ad(30), Ad(40)};
    do {
      DataRange data_range;
      // First write should never overlap anything
      EXPECT_TRUE(data_range.ClaimBytes(starts[0], 12));

      // Second write will overlap unless we are writing the middle last.
      bool success_expected = (starts[2] == Ad(30));
      EXPECT_EQ(data_range.ClaimBytes(starts[1], 12), success_expected);

      // Third write will always overlap something.
      EXPECT_FALSE(data_range.ClaimBytes(starts[2], 12));

      EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(20, 52)));
    } while (std::next_permutation(starts.begin(), starts.end()));
  }
}

TEST(DataRange, ComplexMerge) {
  // check behavior when a merge has more complex consolidation
  // Seed state looks like this:
  //     10   20   30   40   50   60
  //      [---]     [---]     [---]
  DataRange seed_state;
  EXPECT_TRUE(seed_state.ClaimBytes(Ad(10), 10));
  EXPECT_TRUE(seed_state.ClaimBytes(Ad(30), 10));
  EXPECT_TRUE(seed_state.ClaimBytes(Ad(50), 10));

  {
    // Non-overlapping write that merges blocks
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //           [---]
    DataRange data_range = seed_state;
    EXPECT_TRUE(data_range.ClaimBytes(Ad(20), 10));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 40), Ch(50, 60)));
  }

  // Same merges, but overlapping:
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //        [------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(15), 15));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 40), Ch(50, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //           [------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(20), 15));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 40), Ch(50, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //        [---------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(15), 20));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 40), Ch(50, 60)));
  }

  // Writes entirely inside existing areas
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //                [-]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(30), 5));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(10, 20), Ch(30, 40), Ch(50, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //                 [-]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(32), 5));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(10, 20), Ch(30, 40), Ch(50, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //                  [-]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(35), 5));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(10, 20), Ch(30, 40), Ch(50, 60)));
  }

  // Writes that eclipse an entire existing chunk
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //             [------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(25), 15));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(10, 20), Ch(25, 40), Ch(50, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //                [------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(30), 15));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(10, 20), Ch(30, 45), Ch(50, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //             [---------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(25), 20));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(10, 20), Ch(25, 45), Ch(50, 60)));
  }

  // Writes that merge several chunks
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //           [-------------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(20), 30));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //        [-------------------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(15), 40));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 60)));
  }
  {
    //     10   20   30   40   50   60
    //      [---]     [---]     [---]
    //      [-----------------------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(10), 50));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 60)));
  }
  {
    //     10   20   30   40   50   60   70
    //      [---]     [---]     [---]
    //      [----------------------------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(10), 60));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(10, 70)));
  }
  {
    //      0   10   20   30   40   50   60
    //           [---]     [---]     [---]
    //      [----------------------------]
    DataRange data_range = seed_state;
    EXPECT_FALSE(data_range.ClaimBytes(Ad(0), 60));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(0, 60)));
  }
}

TEST(DataRange, BankWrapping) {
  // Test that writing across a bank boundary wraps around
  {
    DataRange data_range;
    EXPECT_TRUE(data_range.ClaimBytes(Ad(0x5fff0), 0x20));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(0x50000, 0x50010), Ch(0x5fff0, 0x60000)));
  }

  // Writing on the edge of a bank boundary should not cause problems
  {
    DataRange data_range;
    EXPECT_TRUE(data_range.ClaimBytes(Ad(0x5fff0), 0x10));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(0x5fff0, 0x60000)));
  }
  {
    DataRange data_range;
    EXPECT_TRUE(data_range.ClaimBytes(Ad(0x50000), 0x10));
    EXPECT_THAT(data_range.Chunks(), ElementsAre(Ch(0x50000, 0x50010)));
  }

  // Wrapping should still merge correctly with existing values
  {
    DataRange data_range;
    EXPECT_TRUE(data_range.ClaimBytes(Ad(0x5ffd0), 0x20));
    EXPECT_TRUE(data_range.ClaimBytes(Ad(0x5fff0), 0x20));
    EXPECT_TRUE(data_range.ClaimBytes(Ad(0x50010), 0x20));
    EXPECT_THAT(data_range.Chunks(),
                ElementsAre(Ch(0x50000, 0x50030), Ch(0x5ffd0, 0x60000)));
  }
}

TEST(RangeMap, SanityTest) {
  RangeMap<int> map;
  DataRange first_range;
  DataRange second_range;
  first_range.ClaimBytes(Ad(10), 10);
  second_range.ClaimBytes(Ad(30), 10);
  first_range.ClaimBytes(Ad(50), 10);
  second_range.ClaimBytes(Ad(70), 10);

  EXPECT_TRUE(map.Insert(first_range, 1));
  EXPECT_TRUE(map.Insert(second_range, 2));

  for (nsasm::Address i = Ad(0); i < Ad(100); i = i.AddWrapped(1)) {
    SCOPED_TRACE(i);
    absl::optional<int> expected;
    if ((i >= Ad(10) && i < Ad(20)) || (i >= Ad(50) && i < Ad(60))) {
      expected = 1;
    }
    if ((i >= Ad(30) && i < Ad(40)) || (i >= Ad(70) && i < Ad(80))) {
      expected = 2;
    }
    EXPECT_EQ(map.Lookup(i), expected);
    EXPECT_EQ(map.Contains(i), expected.has_value());
  }
}

}  // namespace
}  // namespace nsasm
