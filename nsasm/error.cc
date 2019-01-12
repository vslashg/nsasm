#include "error.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string Error::ToString() const {
  if (location_.path.empty()) {
    return message_;
  }
  return absl::StrFormat("%s:0x%x: %s", location_.path, location_.offset, message_);
}

}