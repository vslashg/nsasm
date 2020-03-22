#include "nsasm/statement.h"

namespace nsasm {

std::string Statement::ToString() const {
  switch (data_.index()) {
    case 0:
      return absl::get<nsasm::Instruction>(data_).ToString();
    case 1:
    default:
      return absl::get<nsasm::Directive>(data_).ToString();
  }
}

ErrorOr<void> Statement::Execute(ExecutionState* es) const {
  switch (data_.index()) {
    case 0:
      return absl::get<nsasm::Instruction>(data_).Execute(es);
    case 1:
    default:
      return absl::get<nsasm::Directive>(data_).Execute(es);
  }
}

nsasm::Location Statement::Location() const {
  switch (data_.index()) {
    case 0:
      return absl::get<nsasm::Instruction>(data_).location;
    case 1:
    default:
      return absl::get<nsasm::Directive>(data_).location;
  }
}

int Statement::SerializedSize() const {
  switch (data_.index()) {
    case 0:
      return absl::get<nsasm::Instruction>(data_).SerializedSize();
    case 1:
    default:
      return absl::get<nsasm::Directive>(data_).SerializedSize();
  }
}

ErrorOr<void> Statement::Assemble(nsasm::Address address,
                                  const LookupContext& context,
                                  OutputSink* sink) const {
  switch (data_.index()) {
    case 0:
      return absl::get<nsasm::Instruction>(data_).Assemble(address, context,
                                                           sink);
    case 1:
    default:
      return absl::get<nsasm::Directive>(data_).Assemble(address, context,
                                                         sink);
  }
}

bool Statement::IsExitInstruction() const {
  switch (data_.index()) {
    case 0:
      return absl::get<nsasm::Instruction>(data_).IsExitInstruction();
    case 1:
    default:
      return absl::get<nsasm::Directive>(data_).IsExitInstruction();
  }
}

}  // namespace nsasm
