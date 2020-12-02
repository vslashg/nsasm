#ifndef NSASM_DIRECTIVE_H_
#define NSASM_DIRECTIVE_H_

#include <iostream>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "nsasm/address.h"
#include "nsasm/calling_convention.h"
#include "nsasm/execution_state.h"
#include "nsasm/expression.h"
#include "nsasm/mnemonic.h"
#include "nsasm/output_sink.h"

namespace nsasm {

// The type of argument taken by a given directive
enum DirectiveType {
  DT_no_arg,
  DT_single_arg,
  DT_constant_arg,
  DT_flag_arg,
  DT_calling_convention_arg,
  DT_list_arg,
  DT_name_arg,
  DT_remote_arg,
};

// Returns the type of argument that the given directive accepts.
DirectiveType DirectiveTypeByName(DirectiveName d);

struct Directive {
  DirectiveName name;
  ExpressionOrNull argument;
  StatusFlags flag_state_argument;
  ReturnConvention return_convention_argument;
  std::vector<ExpressionOrNull> list_argument;
  Location location;

  ErrorOr<void> Execute(ExecutionState* state) const;

  int SerializedSize() const;

  // Attempt to assemble this instruction to the given address and given sink.
  //
  // Returns an error if the instruction cannot be assembled for some reason.
  // Also forwards any error returned by the output sink.
  ErrorOr<void> Assemble(nsasm::Address address, const LookupContext& context,
                         OutputSink* sink) const;

  std::string ToString() const;

  bool IsExitInstruction() const { return name == D_halt; }
};

// googletest pretty printers (streams are an abomination)
inline void PrintTo(DirectiveName d, std::ostream* out) {
  auto sv = ToString(d);
  if (sv.empty()) sv = "???";
  *out << sv;
}

inline void PrintTo(DirectiveType t, std::ostream* out) {
  switch (t) {
    case DT_no_arg:
      *out << "DT_no_arg";
      return;
    case DT_single_arg:
      *out << "DT_single_arg";
      return;
    case DT_constant_arg:
      *out << "DT_constant_arg";
      return;
    case DT_flag_arg:
      *out << "DT_flag_arg";
      return;
    case DT_calling_convention_arg:
      *out << "DT_calling_convention_arg";
      return;
    case DT_list_arg:
      *out << "DT_list_arg";
      return;
    case DT_name_arg:
      *out << "DT_name_arg";
      return;
    case DT_remote_arg:
      *out << "DT_remote_arg";
      return;
    default:
      *out << "???";
      return;
  }
}

}  // namespace nsasm

#endif  // NSASM_DIRECTIVE_H_
