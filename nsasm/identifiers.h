#ifndef NSASM_IDENTIFIERS_H_
#define NSASM_IDENTIFIERS_H_

#include <string>
#include <tuple>

#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"

namespace nsasm {

class FullIdentifier {
 public:
  FullIdentifier(std::string mod_name, std::string id_name)
      : mod_name_(std::move(mod_name)), id_name_(std::move(id_name)) {}
  FullIdentifier(std::string id_name)
      : mod_name_(absl::nullopt), id_name_(std::move(id_name)) {}
  FullIdentifier(const FullIdentifier&) = default;
  FullIdentifier(FullIdentifier&&) = default;

  const absl::optional<std::string>& OptionalModule() const { return mod_name_; }
  bool Qualified() const { return mod_name_.has_value(); }
  const std::string& Module() const { return *mod_name_; }
  const std::string& Identifier() const { return id_name_; }
  const std::string ToString() const {
    if (Qualified()) {
      return absl::StrCat(*mod_name_, "::", id_name_);
    } else {
      return id_name_;
    }
  }

  bool operator==(const FullIdentifier& rhs) const {
    return std::tie(mod_name_, id_name_) == std::tie(rhs.mod_name_, rhs.id_name_);
  }
  bool operator!=(const FullIdentifier& rhs) const {
    return std::tie(mod_name_, id_name_) != std::tie(rhs.mod_name_, rhs.id_name_);
  }
  /*
  bool operator<(const FullIdentifier& rhs) const {
    return std::tie(mod_name_, id_name_) < std::tie(rhs.mod_name_, rhs.id_name_);
  }
  bool operator<=(const FullIdentifier& rhs) const {
    return std::tie(mod_name_, id_name_) <= std::tie(rhs.mod_name_, rhs.id_name_);
  }
  bool operator>(const FullIdentifier& rhs) const {
    return std::tie(mod_name_, id_name_) > std::tie(rhs.mod_name_, rhs.id_name_);
  }
  bool operator>=(const FullIdentifier& rhs) const {
    return std::tie(mod_name_, id_name_) >= std::tie(rhs.mod_name_, rhs.id_name_);
  }
  */

  template <typename H>
  friend H AbslHashValue(H h, const FullIdentifier& n) {
    return H::combine(std::move(h), n.mod_name_, n.id_name_);
  }

 private:
  absl::optional<std::string> mod_name_;
  std::string id_name_;
};

}  // namespace nsasm

#endif  // NSASM_IDENTIFIERS_H_
