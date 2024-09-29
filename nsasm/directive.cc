#include "nsasm/directive.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "nsasm/memory.h"

namespace nsasm {

DirectiveType DirectiveTypeByName(DirectiveName d) {
  static auto lookup = new absl::flat_hash_map<DirectiveName, DirectiveType>{
      {D_begin, DT_no_arg},     {D_db, DT_list_arg},
      {D_dl, DT_list_arg},      {D_dw, DT_list_arg},
      {D_end, DT_no_arg},       {D_entry, DT_calling_convention_arg},
      {D_equ, DT_single_arg},   {D_halt, DT_no_arg},
      {D_mode, DT_flag_arg},    {D_module, DT_name_arg},
      {D_org, DT_constant_arg}, {D_remote, DT_remote_arg},
  };
  auto iter = lookup->find(d);
  if (iter == lookup->end()) {
    return DT_single_arg;  // ???
  }
  return iter->second;
}

std::string Directive::ToString() const {
  DirectiveType type = DirectiveTypeByName(name);
  switch (type) {
    case DT_no_arg:
      return absl::StrCat(nsasm::ToString(name));
    case DT_single_arg:
    case DT_constant_arg:
    case DT_name_arg:
      return absl::StrCat(nsasm::ToString(name), " ", argument.ToString());
    case DT_flag_arg:
    case DT_calling_convention_arg:
      return absl::StrCat(nsasm::ToString(name), " ",
                          flag_state_argument.ToName(),
                          return_convention_argument.ToSuffixString());
    case DT_list_arg:
      return absl::StrCat(
          nsasm::ToString(name), " ",
          absl::StrJoin(list_argument, ", ",
                        [](std::string* out, const ExpressionOrNull& v) {
                          out->append(v.ToString());
                        }));
    case DT_remote_arg:
      return absl::StrCat(nsasm::ToString(name), " ", argument.ToString(), " ",
                          flag_state_argument.ToName());
  }
  return "???";
}

ErrorOr<void> Directive::Execute(ExecutionState* state) const {
  if (name == D_db || name == D_dl || name == D_dw || name == D_org) {
    // Attempt to execute data or across .ORG gap
    return Error("Execution continues into %s directive",
                 nsasm::ToString(name));
  }
  if (name == D_mode) {
    // The .mode directive forces static analysis to change its state to its
    // argument
    state->Flags() = flag_state_argument;
  }
  // Other directives are no-ops in analysis
  return {};
}

int Directive::SerializedSize() const {
  int bytes_per_entry = 0;
  if (name == D_db) {
    bytes_per_entry = 1;
  } else if (name == D_dw) {
    bytes_per_entry = 2;
  } else if (name == D_dl) {
    bytes_per_entry = 3;
  }
  return bytes_per_entry * list_argument.size();
}

namespace {
void EncodeValue(DirectiveName d, int value, std::vector<uint8_t>* buf) {
  buf->push_back(value & 0xff);
  if (d == D_dw || d == D_dl) {
    buf->push_back((value >> 8) & 0xff);
  }
  if (d == D_dl) {
    buf->push_back((value >> 16) & 0xff);
  }
}
}  // namespace

ErrorOr<void> Directive::Assemble(nsasm::Address address,
                                  const LookupContext& context,
                                  OutputSink* sink) const {
  if (name != D_db && name != D_dw && name != D_dl) {
    return {};
  }
  std::vector<uint8_t> bytes;
  for (const ExpressionOrNull& expr : list_argument) {
    auto value = expr.Evaluate(context);
    NSASM_RETURN_IF_ERROR(value);
    EncodeValue(name, *value, &bytes);
  }
  return sink->Write(address, bytes);
}

}  // namespace nsasm
