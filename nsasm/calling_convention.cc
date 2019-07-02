#include "nsasm/calling_convention.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string ReturnConvention::ToSuffixString() const {
  switch (state_.index()) {
    case 0:
    default:
      return "";  // default
    case 1:
      return absl::StrFormat(" YIELDS %s",
                             absl::get<FlagState>(state_).ToString());
    case 2:
      return " NORETURN";
  }
}

}