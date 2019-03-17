#include "nsasm/expression.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

namespace nsasm {

std::string Literal::ToString() const {
  int output_value = CastTo(type_, value_);
  switch (type_) {
    case T_byte:
      return absl::StrFormat("$%02x", output_value);
    case T_word:
      return absl::StrFormat("$%04x", output_value);
    case T_long:
      return absl::StrFormat("$%06x", output_value);
    default:
      return absl::StrFormat("%d", output_value);
  }
}

std::string Identifier::ToString() const {
  const char* prefix = (type_ == T_long) ? "@" : "";
  if (module_.empty()) {
    return absl::StrCat(prefix, identifier_);
  } else {
    return absl::StrCat(prefix, module_, "::", identifier_);
  }
}

}  // namespace nsasm