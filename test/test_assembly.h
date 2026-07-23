#ifndef TEST_TEST_ASSEMBLY_H_
#define TEST_TEST_ASSEMBLY_H_

#include "absl/types/optional.h"
#include "nsasm/assembler.h"
#include "test/test_sink.h"

namespace nsasm {

// Test that the given set of ASM file contents result in assembling to the
// given set of bytes.
void ExpectAssembly(const std::vector<std::string_view>& asm_contents,
                    std::vector<ExpectedBytes> expected);

// Test that the given ASM file contents result in assembling to the given set
// of bytes.
inline void ExpectAssembly(std::string_view asm_contents,
                           std::vector<ExpectedBytes> expected) {
  return ExpectAssembly(std::vector<std::string_view>{asm_contents},
                        std::move(expected));
}

// Test that the given set of ASM file contents results in an assembly error.
// message, if provided, is a subset expected to be found in the error message.
void ExpectAssemblyError(const std::vector<std::string_view>& asm_contents,
                         std::string_view message = "");

inline void ExpectAssemblyError(std::string_view asm_contents,
                                std::string_view message = "") {
  return ExpectAssemblyError(std::vector<std::string_view>{asm_contents},
                             message);
}

// Assembles the given sources (optionally checking output bytes, as
// ExpectAssembly does) and returns the Assembler for post-assembly queries
// (JumpTargets(), JumpTargetReturnConventions(), NameForAddress(), ...).
// Adds a test failure and returns nullopt on error.
absl::optional<Assembler> AssembleForTest(
    const std::vector<std::string_view>& asm_contents,
    std::vector<ExpectedBytes> expected = {});

inline absl::optional<Assembler> AssembleForTest(
    std::string_view asm_contents, std::vector<ExpectedBytes> expected = {}) {
  return AssembleForTest(std::vector<std::string_view>{asm_contents},
                         std::move(expected));
}

}  // namespace nsasm

#endif  // TEST_TEST_ASSEMBLY_H_
