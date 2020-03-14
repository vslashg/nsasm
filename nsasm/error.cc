#include "error.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string Error::ToString() const {
  std::string loc_str = location_.ToString();
  if (loc_str.empty()) {
    return message_;
  }
  return absl::StrFormat("%s: %s", loc_str, message_);
}

}  // namespace nsasm