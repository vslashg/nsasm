#ifndef NSASM_TOKEN_H_
#define NSASM_TOKEN_H_

#include <string>

#include "absl/types/variant.h"
#include "nsasm/error.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

enum NumericType {
  T_unknown,
  T_byte,
  T_word,
  T_long,
};

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

 private:
  absl::variant<absl::monostate, std::string, int, Mnemonic, char> value_;
  Location location_;
  NumericType type_ = T_unknown;
};

ErrorOr<std::vector<Token>> Tokenize(absl::string_view);

}  // namespace nsasm

#endif  // NSASM_TOKEN_H_
