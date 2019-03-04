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

int Statement::SerializedSize() const {
  return absl::visit([](const auto& v) { return v.SerializedSize(); }, data_);
}

ErrorOr<void> Statement::Assemble(int address, const LookupContext& context,
                                  OutputSink* sink) const {
  return absl::visit(
      [address, &context, sink](const auto& v) {
        return v.Assemble(address, context, sink);
      },
      data_);
}

}  // namespace nsasm
