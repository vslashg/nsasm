#ifndef TEST_TEST_ASSEMBLY_H_
#define TEST_TEST_ASSEMBLY_H_

#include "test/test_sink.h"

namespace nsasm {

// Test that the given set of ASM file contents result in assembling to the
// given set of bytes.
void ExpectAssembly(const std::vector<absl::string_view>& asm_contents,
                    std::vector<ExpectedBytes> expected);

// Test that the given ASM file contents result in assembling to the given set
// of bytes.
inline void ExpectAssembly(absl::string_view asm_contents,
                           std::vector<ExpectedBytes> expected) {
  return ExpectAssembly(std::vector<absl::string_view>{asm_contents},
                        std::move(expected));
}

// Test that the given set of ASM file contents results in an assembly error.
// message, if provided, is a subset expected to be found in the error message.
void ExpectAssemblyError(const std::vector<absl::string_view>& asm_contents,
                         absl::string_view message = "");

inline void ExpectAssemblyError(absl::string_view asm_contents,
                                absl::string_view message = "") {
  return ExpectAssemblyError(std::vector<absl::string_view>{asm_contents},
                             message);
}

}  // namespace nsasm

#endif  // TEST_TEST_ASSEMBLY_H_
