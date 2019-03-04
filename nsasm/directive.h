#ifndef NSASM_DIRECTIVE_H_
#define NSASM_DIRECTIVE_H_

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "nsasm/expression.h"
#include "nsasm/flag_state.h"
#include "nsasm/output_sink.h"

namespace nsasm {

// All assembler directives understood by nsasm
enum DirectiveName {
  D_db,
  D_dl,
  D_dw,
  D_entry,
  D_equ,
  D_mode,
  D_org,
};

// The type of argument taken by a given directive
enum DirectiveType {
  DT_single_arg,
  DT_constant_arg,
  DT_flag_arg,
  DT_list_arg,
};

// Conversions between DirectiveName values, and the matching strings.
absl::string_view ToString(DirectiveName d);
absl::optional<DirectiveName> ToDirectiveName(std::string s);

// Returns the type of argument that the given directive accepts.
DirectiveType DirectiveTypeByName(DirectiveName d);

struct Directive {
  DirectiveName name;
  ExpressionOrNull argument;
  FlagState flag_state_argument;
  std::vector<ExpressionOrNull> list_argument;
  Location location;

  ErrorOr<FlagState> Execute(const FlagState& state) const;

  int SerializedSize() const;

  // Attempt to assemble this instruction to the given address and given sink.
  //
  // Returns an error if the instruction cannot be assembled for some reason.
  // Also forwards any error returned by the output sink.
  ErrorOr<void> Assemble(int address, const LookupContext& context,
                         OutputSink* sink) const;

  std::string ToString() const;
};



// googletest pretty printers (streams are an abomination)
inline void PrintTo(DirectiveName d, std::ostream* out) {
  auto sv = ToString(d);
  if (sv.empty()) sv = "???";
  *out << sv;
}

inline void PrintTo(DirectiveType t, std::ostream* out) {
  switch (t) {
    case DT_single_arg:
      *out << "DT_single_arg";
      return;
    case DT_constant_arg:
      *out << "DT_constant_arg";
      return;
    case DT_flag_arg:
      *out << "DT_flag_arg";
      return;
    case DT_list_arg:
      *out << "DT_list_arg";
      return;
    default:
      *out << "???";
      return;
  }
}

}  // namespace nsasm

#endif  // NSASM_DIRECTIVE_H_
