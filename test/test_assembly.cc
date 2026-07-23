#include "test/test_assembly.h"

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "nsasm/assembler.h"
#include "nsasm/file.h"
#include "test/test_sink.h"

namespace nsasm {

void ExpectAssembly(const std::vector<std::string_view>& asm_contents,
                    std::vector<ExpectedBytes> expected) {
  TestSink test_sink(std::move(expected));
  std::vector<File> files;
  for (size_t i = 0; i < asm_contents.size(); ++i) {
    files.push_back(
        MakeFakeFile(absl::StrCat("fake_file_", i, ".asm"), asm_contents[i]));
  }

  auto assembler = Assemble(files, &test_sink);
  if (!assembler.ok()) {
    ADD_FAILURE() << assembler.error().ToString();
    return;
  }

  auto result = test_sink.Check();
  if (!result.ok()) {
    ADD_FAILURE() << result.error().ToString();
  }
}

absl::optional<Assembler> AssembleForTest(
    const std::vector<std::string_view>& asm_contents,
    std::vector<ExpectedBytes> expected) {
  bool check_bytes = !expected.empty();
  TestSink test_sink(std::move(expected));
  std::vector<File> files;
  for (size_t i = 0; i < asm_contents.size(); ++i) {
    files.push_back(
        MakeFakeFile(absl::StrCat("fake_file_", i, ".asm"), asm_contents[i]));
  }

  auto assembler = Assemble(files, &test_sink);
  if (!assembler.ok()) {
    ADD_FAILURE() << assembler.error().ToString();
    return absl::nullopt;
  }

  if (check_bytes) {
    auto result = test_sink.Check();
    if (!result.ok()) {
      ADD_FAILURE() << result.error().ToString();
      return absl::nullopt;
    }
  }
  return *std::move(assembler);
}

void ExpectAssemblyError(const std::vector<std::string_view>& asm_contents,
                         std::string_view message) {
  TestSink test_sink({});
  std::vector<File> files;
  for (size_t i = 0; i < asm_contents.size(); ++i) {
    files.push_back(
        MakeFakeFile(absl::StrCat("fake_file_", i, ".asm"), asm_contents[i]));
  }

  auto assembler = Assemble(files, &test_sink);
  if (assembler.ok()) {
    ADD_FAILURE() << "Unexpected successful assembly";
    return;
  }
  if (!absl::StrContains(assembler.error().ToString(), message)) {
    ADD_FAILURE() << "Expected `" << message << "` in error message, got: "
                  << assembler.error().ToString();
  }
}

}  // namespace nsasm
