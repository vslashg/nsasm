#include "nsasm/directive.h"

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"

namespace nsasm {
namespace {

constexpr absl::string_view directive_names[] = {
    ".DB", ".DL", ".DW", ".ENTRY", ".EQU", ".MODE", ".ORG",
};

}  // namespace

absl::string_view ToString(DirectiveName d) {
  if (d < D_db || d > D_org) {
    return "";
  }
  return directive_names[d];
}

absl::optional<DirectiveName> ToDirectiveName(std::string s) {
  static auto lookup =
      new absl::flat_hash_map<absl::string_view, DirectiveName>{
          {".DB", D_db},       {".DL", D_dl},   {".DW", D_dw},
          {".ENTRY", D_entry}, {".EQU", D_equ}, {".MODE", D_mode},
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
      {D_db, DT_list_arg},    {D_dl, DT_list_arg},    {D_dw, DT_list_arg},
      {D_entry, DT_flag_arg}, {D_equ, DT_single_arg}, {D_mode, DT_flag_arg},
      {D_org, DT_single_arg},
  };
  auto iter = lookup->find(d);
  if (iter == lookup->end()) {
    return DT_single_arg;  // ???
  }
  return iter->second;
}

}  // namespace nsasm