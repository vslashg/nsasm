#include "nsasm/directive.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "nsasm/output_sink.h"

namespace nsasm {
namespace {

constexpr absl::string_view directive_names[] = {
    ".BEGIN", ".DB",  ".DL",   ".DW",     ".END",
    ".ENTRY", ".EQU", ".MODE", ".MODULE", ".ORG",
};

}  // namespace

absl::string_view ToString(DirectiveName d) {
  if (d < D_begin || d > D_org) {
    return "";
  }
  return directive_names[d];
}

absl::optional<DirectiveName> ToDirectiveName(std::string s) {
  static auto lookup =
      new absl::flat_hash_map<absl::string_view, DirectiveName>{
          {".BEGIN", D_begin}, {".DB", D_db},     {".DL", D_dl},
          {".DW", D_dw},       {".END", D_end},   {".ENTRY", D_entry},
          {".EQU", D_equ},     {".MODE", D_mode}, {".MODULE", D_module},
          {".ORG", D_org},
      };
  absl::AsciiStrToUpper(&s);
  auto iter = lookup->find(s);
  if (iter == lookup->end()) {
    return absl::nullopt;
  }
  return iter->second;
}

DirectiveType DirectiveTypeByName(DirectiveName d) {
  static auto lookup = new absl::flat_hash_map<DirectiveName, DirectiveType>{
      {D_begin, DT_no_arg},     {D_db, DT_list_arg},   {D_dl, DT_list_arg},
      {D_dw, DT_list_arg},      {D_end, DT_no_arg},    {D_entry, DT_flag_arg},
      {D_equ, DT_single_arg},   {D_mode, DT_flag_arg}, {D_module, DT_name_arg},
      {D_org, DT_constant_arg},
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
      return absl::StrCat(nsasm::ToString(name), " ",
                          flag_state_argument.ToName());
    case DT_list_arg:
      return absl::StrCat(
          nsasm::ToString(name), " ",
          absl::StrJoin(list_argument, ", ",
                        [](std::string* out, const ExpressionOrNull& v) {
                          out->append(v.ToString());
                        }));
  }
  return "???";
}

ErrorOr<FlagState> Directive::Execute(const FlagState& state) const {
  if (name == D_db || name == D_dl || name == D_dw || name == D_org) {
    // Attempt to execute data or across .ORG gap
    return Error("Execution continues into %s directive",
                 nsasm::ToString(name));
  }
  if (name == D_mode) {
    // The .mode directive forces static analysis to change its state to its
    // argument
    return flag_state_argument;
  }
  // Other directives are no-ops in analysis
  return state;
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

ErrorOr<void> Directive::Assemble(int address, const LookupContext& context,
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
