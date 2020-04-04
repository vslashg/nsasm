#include "test/test_assembly.h"

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "nsasm/assembler.h"
#include "nsasm/file.h"
#include "test/test_sink.h"

namespace nsasm {

void ExpectAssembly(const std::vector<absl::string_view>& asm_contents,
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

void ExpectAssemblyError(const std::vector<absl::string_view>& asm_contents,
                         absl::string_view message) {
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
