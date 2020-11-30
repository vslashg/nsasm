#ifndef NSASM_PARSE_H_
#define NSASM_PARSE_H_

#include "absl/types/variant.h"
#include "nsasm/error.h"
#include "nsasm/statement.h"
#include "nsasm/token.h"

namespace nsasm {

class ParsedLabel {
 public:
  ParsedLabel() {}
  ParsedLabel(nsasm::Punctuation p) : plus_or_minus_(p) {};
  ParsedLabel(std::string n, bool e) : name_(std::move(n)), exported_(e) {};

  bool IsPlusOrMinus() const { return plus_or_minus_ != P_none; }
  bool IsIdentifier() const { return plus_or_minus_ == P_none; }
  bool IsExported() const { return exported_; }
  Punctuation PlusOrMinus() const { return plus_or_minus_; }
  const std::string Identifier() const { return name_; }

  std::string ToString() const;
 private:
  nsasm::Punctuation plus_or_minus_ = P_none;
  std::string name_;
  bool exported_ = false;
};

// Parses a sequence of tokens into a sequence of statements and labels.
// These tokens are assumed to be from a single line of code.
ErrorOr<std::vector<absl::variant<Statement, ParsedLabel>>> Parse(
    absl::Span<const Token> tokens);

// Parse a string into an expression object.  Intended for testing purposes.
ErrorOr<ExpressionOrNull> ParseExpression(absl::string_view);

}  // namespace nsasm

#endif  // NSASM_PARSE_H_
