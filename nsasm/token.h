#ifndef NSASM_TOKEN_H_
#define NSASM_TOKEN_H_

#include <iostream>
#include <string>

#include "absl/types/variant.h"
#include "nsasm/directive.h"
#include "nsasm/error.h"
#include "nsasm/mnemonic.h"
#include "nsasm/numeric_type.h"

namespace nsasm {

struct EndOfLine {
  bool operator==(EndOfLine rhs) const { return true; }
  bool operator!=(EndOfLine rhs) const { return false; }
};

enum Punctuation {
  P_none = 0,
  P_scope = 257,
  P_export = 258,
  P_noreturn = 259,
  P_yields = 260,
  P_plusplus = 261,
  P_plusplusplus = 262,
  P_minusminus = 263,
  P_minusminusminus = 264,
};

class Token {
 public:
  explicit Token(const std::string& identifier, Location loc)
      : value_(identifier), location_(loc) {}
  explicit Token(int number, Location loc, NumericType type = T_unknown)
      : value_(number), location_(loc), type_(type) {}
  explicit Token(nsasm::Mnemonic mnemonic, Location loc)
      : value_(mnemonic), location_(loc) {}
  explicit Token(nsasm::Suffix suffix, Location loc)
      : value_(suffix), location_(loc) {}
  explicit Token(nsasm::DirectiveName directive_name, Location loc)
      : value_(directive_name), location_(loc) {}
  explicit Token(char punctuation, Location loc)
      : value_(nsasm::Punctuation(punctuation)), location_(loc) {}
  explicit Token(nsasm::Punctuation punctuation, Location loc)
      : value_(punctuation), location_(loc) {}
  explicit Token(nsasm::EndOfLine eol, Location loc)
      : value_(eol), location_(loc) {}

  const std::string* Identifier() const {
    return absl::get_if<std::string>(&value_);
  }
  const int* Literal() const { return absl::get_if<int>(&value_); }
  const nsasm::Mnemonic* Mnemonic() const {
    return absl::get_if<nsasm::Mnemonic>(&value_);
  }
  const nsasm::Suffix* Suffix() const {
    return absl::get_if<nsasm::Suffix>(&value_);
  }
  const nsasm::DirectiveName* DirectiveName() const {
    return absl::get_if<nsasm::DirectiveName>(&value_);
  }
  const nsasm::Punctuation* Punctuation() const {
    return absl::get_if<nsasm::Punctuation>(&value_);
  }
  const nsasm::EndOfLine* EndOfLine() const {
    return absl::get_if<nsasm::EndOfLine>(&value_);
  }

  NumericType Type() const { return type_; }
  const nsasm::Location& Location() const { return location_; }

  // Stringize this token, for use in error messages
  std::string ToString() const;

  // Equality compares raw value, but not location or bit width.
  // Intended for testing and parser writing.
  bool operator==(const Token& rhs) const { return value_ == rhs.value_; }
  bool operator!=(const Token& rhs) const { return value_ != rhs.value_; }

 private:
  absl::variant<std::string, int, nsasm::Mnemonic, nsasm::Suffix,
                nsasm::DirectiveName, nsasm::Punctuation, nsasm::EndOfLine>
      value_;
  nsasm::Location location_;
  NumericType type_ = T_unknown;
};

using TokenSpan = absl::Span<const nsasm::Token>;

ErrorOr<std::vector<Token>> Tokenize(absl::string_view, Location loc);

// Convenience comparisons.  Tokens cannot be created from values implicitly,
// but can be compared for equality with those objects.
inline bool operator==(const std::string& lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(const std::string& lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, const std::string& rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, const std::string& rhs) {
  return lhs != Token(rhs, Location());
}

inline bool operator==(int lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(int lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, int rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, int rhs) {
  return lhs != Token(rhs, Location());
}

inline bool operator==(nsasm::Mnemonic lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(nsasm::Mnemonic lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, nsasm::Mnemonic rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, nsasm::Mnemonic rhs) {
  return lhs != Token(rhs, Location());
}

inline bool operator==(nsasm::Suffix lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(nsasm::Suffix lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, nsasm::Suffix rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, nsasm::Suffix rhs) {
  return lhs != Token(rhs, Location());
}

inline bool operator==(nsasm::DirectiveName lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(nsasm::DirectiveName lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, nsasm::DirectiveName rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, nsasm::DirectiveName rhs) {
  return lhs != Token(rhs, Location());
}

inline bool operator==(char lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(char lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, char rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, char rhs) {
  return lhs != Token(rhs, Location());
}

inline bool operator==(nsasm::Punctuation lhs, const Token& rhs) {
  return Token(lhs, Location()) == rhs;
}
inline bool operator!=(nsasm::Punctuation lhs, const Token& rhs) {
  return Token(lhs, Location()) != rhs;
}
inline bool operator==(const Token& lhs, nsasm::Punctuation rhs) {
  return lhs == Token(rhs, Location());
}
inline bool operator!=(const Token& lhs, nsasm::Punctuation rhs) {
  return lhs != Token(rhs, Location());
}

// googletest pretty printer (streams suck)
inline void PrintTo(const Token& v, std::ostream* out) { *out << v.ToString(); }

}  // namespace nsasm

#endif  // NSASM_TOKEN_H_
