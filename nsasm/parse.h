#ifndef NSASM_PARSE_H_
#define NSASM_PARSE_H_

#include "nsasm/directive.h"
#include "nsasm/error.h"
#include "nsasm/instruction.h"
#include "nsasm/token.h"

#include "absl/types/variant.h"

namespace nsasm {

// Parses a sequence of tokens into a sequence of instructions and labels.
// These tokens are assumed to be from a single line of code.
ErrorOr<std::vector<absl::variant<Instruction, Directive, std::string>>> Parse(
    absl::Span<const Token> tokens);

}  // namespace nsasm

#endif  // NSASM_PARSE_H_