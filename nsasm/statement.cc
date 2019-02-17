#include "nsasm/statement.h"

namespace nsasm {

std::string Statement::ToString() const {
  return absl::visit([](const auto& v) { return v.ToString(); }, data_);
}

}  // namespace nsasm
