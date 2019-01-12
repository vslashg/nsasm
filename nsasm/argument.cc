#include "nsasm/argument.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string Argument::ToString(int argument_size) const {
  if (!label_.empty()) {
    return label_;
  }
  if (!value_.has_value()) {
    return "???";
  }
  switch (argument_size) {
    case 1:
      return absl::StrFormat("$%02x", (*value_ & 0xff));
    case 2:
      return absl::StrFormat("$%04x", (*value_ & 0xffff));
    case 3:
      return absl::StrFormat("$%06x", (*value_ & 0xffffff));
    default:
      return absl::StrFormat("%d", *value_);
  }
}

std::string Argument::ToBranchOffset() const {
  if (!label_.empty()) {
    return label_;
  }
  if (!value_.has_value()) {
    return "???";
  }
  return absl::StrFormat("%d", *value_);
}

}  // namespace nsasm
