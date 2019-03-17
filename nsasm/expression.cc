#include "nsasm/expression.h"

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

}  // namespace nsasm