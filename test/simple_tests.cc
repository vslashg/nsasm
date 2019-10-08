#include "gtest/gtest.h"

#include "test/test_assembly.h"

TEST(SimpleTest, Example) {
  // Test that a simple assembly operation works
  nsasm::ExpectAssembly(
      R"(
      .org $008000
      .entry m8x8
      RTS
      )",
      {{0x8000, {0x60}}});

  // Test that error reporting works
  nsasm::ExpectAssemblyError(
      R"(
      .entry m8x8
      RTS
      )",
      "No address given for assembly");
}
