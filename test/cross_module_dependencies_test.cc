#include <string>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "test/test_assembly.h"
#include "test/test_sink.h"

namespace nsasm {

TEST(CrossModuleDependencies, ValidModues) {
  std::string f1 =
      ".module M1\n"
      "v1 .equ 1\n";
  std::string f2 =
      ".module M2\n"
      "v2 .equ M1::v1 + 1\n";
  std::string f3 =
      ".module M3\n"
      "v3 .equ M2::v2 + 1\n"
      "v4 .equ M1::v1 + 3\n";
  // And a fourth simple module to output the constants defined this way
  std::string f4 =
      ".org $8000\n"
      ".db <M1::v1, <M2::v2, <M3::v3, <M3::v4\n";

  const ExpectedBytes expected{
      .location = 0x8000,
      .bytes = {0x01, 0x02, 0x03, 0x04},
  };

  std::vector<std::string_view> files = {f1, f2, f3, f4};
  std::sort(files.begin(), files.end());
  // These files should all assemble no matter what order they appear in.
  do {
    ExpectAssembly(files, {expected});
  } while (std::next_permutation(files.begin(), files.end()));
}

TEST(CrossModuleDependencies, AnonymousModules) {
  std::string f1 =
      "v1 .equ 1\n";
  std::string f2 =
      "v2 .equ v1 + 1\n";
  std::string f3 =
      "v3 .equ v2 + 1\n"
      "v4 .equ v1 + 3\n";
  std::string f4 =
      ".org $8000\n"
      ".db <v1, <v2, <v3, <v4\n";

  const ExpectedBytes expected{
      .location = 0x8000,
      .bytes = {0x01, 0x02, 0x03, 0x04},
  };

  std::vector<std::string_view> files = {f1, f2, f3, f4};
  std::sort(files.begin(), files.end());
  // These files should all assemble no matter what order they appear in.
  do {
    ExpectAssembly(files, {expected});
  } while (std::next_permutation(files.begin(), files.end()));
}

TEST(CrossModuleDependencies, CyclicDependencies) {
  std::string f1 =
      "v1 .equ v2\n";
  std::string f2 =
      "v2 .equ v1\n";

  std::vector<std::string_view> files = {f1, f2};
    ExpectAssemblyError(files, "Cyclic dependency");
}

}  // namespace nsasm