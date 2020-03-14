#ifndef NSASM_LOCATION_H_
#define NSASM_LOCATION_H_

#include <string>
#include <utility>

#include "absl/strings/str_format.h"

namespace nsasm {

// Representation of a position in a file
class Location {
 public:
  Location() {}
  Location(std::string path) : path_(std::move(path)) {}
  Location(int line_number) : offset_(line_number), offset_type_(kLineNumber) {}
  static Location FromAddress(int address) {
    Location loc;
    loc.offset_type_ = kAddress;
    loc.offset_ = address;
    return loc;
  }

  void Update(const Location& rhs) {
    if (!rhs.path_.empty()) {
      path_ = rhs.path_;
    }
    if (!rhs.offset_type_ != kNone) {
      offset_ = rhs.offset_;
      offset_type_ = rhs.offset_type_;
    }
  }

  std::string ToString() const {
    if (path_.empty()) {
      return {};
    }
    switch (offset_type_) {
      case kNone:
      default:
        return path_;
      case kLineNumber:
        return absl::StrFormat("%s:%d", path_, offset_);
      case kAddress:
        return absl::StrFormat("%s:0x%06x", path_, offset_);
    }
  }

 private:
  enum OffsetType {
    kNone,
    kLineNumber,
    kAddress,
  };

  std::string path_;
  int offset_ = 0;
  OffsetType offset_type_ = kNone;
};

}  // namespace nsasm

#endif  // NSASM_LOCATION_H_
