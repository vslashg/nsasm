#ifndef NSASM_TOKEN_H_
#define NSASM_TOKEN_H_

#include <string>

#include "absl/types/variant.h"
#include "nsasm/error.h"
#include "nsasm/mnemonic.h"
#include "nsasm/numeric_type.h"

namespace nsasm {

class Token {
 public:
  explicit Token(const std::string& identifier, Location loc)
      : value_(identifier), location_(loc) {}
  explicit Token(int number, Location loc, NumericType type = T_unknown)
      : value_(number), location_(loc), type_(type) {}
  explicit Token(Mnemonic mnemonic, Location loc)
      : value_(mnemonic), location_(loc) {}
  explicit Token(char punctuation, Location loc)
      : value_(punctuation), location_(loc) {}

  const NumericType Type() const { return type_; }

  // Equality compares raw value, but not location or bit width.
  // Intended for testing and parser writing.
  bool operator==(const Token& rhs) const {
    return value_ == rhs.value_;
  }
  bool operator!=(const Token& rhs) const {
    return value_ != rhs.value_;
  }

 private:
  absl::variant<std::string, int, Mnemonic, char> value_;
  Location location_;
  NumericType type_ = T_unknown;
};

ErrorOr<std::vector<Token>> Tokenize(absl::string_view, Location loc);

// Convenience comparisons.  Tokens cannot be created from values implicitly,
// but can be compared for equality with those objects.
bool operator==(const std::string& lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
bool operator!=(const std::string& lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
bool operator==(const Token& lhs, const std::string& rhs) {
  return lhs == Token(rhs, Location());
}
bool operator!=(const Token& lhs, const std::string& rhs) {
  return lhs != Token(rhs, Location());
}

bool operator==(int lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
bool operator!=(int lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
bool operator==(const Token& lhs, int rhs) {
  return lhs == Token(rhs, Location());
}
bool operator!=(const Token& lhs, int rhs) {
  return lhs != Token(rhs, Location());
}

bool operator==(Mnemonic lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
bool operator!=(Mnemonic lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
bool operator==(const Token& lhs, Mnemonic rhs) {
  return lhs == Token(rhs, Location());
}
bool operator!=(const Token& lhs, Mnemonic rhs) {
  return lhs != Token(rhs, Location());
}

bool operator==(char lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
bool operator!=(char lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
bool operator==(const Token& lhs, char rhs) {
  return lhs == Token(rhs, Location());
}
bool operator!=(const Token& lhs, char rhs) {
  return lhs != Token(rhs, Location());
}


}  // namespace nsasm

#endif  // NSASM_TOKEN_H_
