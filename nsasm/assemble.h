#ifndef NSASM_ASSEMBLE_H_
#define NSASM_ASSEMBLE_H_

#include "nsasm/error.h"
#include "nsasm/instruction.h"
#include "nsasm/token.h"

#include "absl/types/variant.h"

namespace nsasm {

// Assembles a sequence of instructions and labels from a sequence of tokens.
// These tokens are assumed to be from a single line of code.
ErrorOr<std::vector<absl::variant<Instruction, std::string>>> Assemble(
    absl::Span<const Token> tokens);

}  // namespace nsasm

#endif  // NSASM_ASSEMBLE_H_