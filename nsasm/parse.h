#ifndef NSASM_PARSE_H_
#define NSASM_PARSE_H_

#include "absl/types/variant.h"
#include "nsasm/error.h"
#include "nsasm/statement.h"
#include "nsasm/token.h"

namespace nsasm {

// Parses a sequence of tokens into a sequence of statements and labels.
// These tokens are assumed to be from a single line of code.
//
// TODO: Label should be its own type (rather than std::string), so that
// we can attach a location to it.
ErrorOr<std::vector<absl::variant<Statement, std::string>>> Parse(
    absl::Span<const Token> tokens);

// Parse a string into an expression object.  Intended for testing purposes.
ErrorOr<ExpressionOrNull> ParseExpression(absl::string_view);

}  // namespace nsasm

#endif  // NSASM_PARSE_H_
