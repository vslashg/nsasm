#include "nsasm/statement.h"

namespace nsasm {

std::string Statement::ToString() const {
  return absl::visit([](const auto& v) { return v.ToString(); }, data_);
}

ErrorOr<FlagState> Statement::Execute(const FlagState& fs) const {
  return absl::visit([&fs](const auto& v) { return v.Execute(fs); }, data_);
}

nsasm::Location Statement::Location() const {
  return absl::visit([](const auto& v) { return v.location; }, data_);
}

}  // namespace nsasm
