#include "nsasm/disassemble.h"

#include <string>

#include "absl/strings/str_cat.h"

namespace nsasm {

absl::optional<Disassembly> Disassemble(const Rom& rom, int starting_address,
                                        FlagState initial_flag_state) {
  int gensym_count = 0;
  auto gensym =
      [&gensym_count]() { return absl::StrCat("gensym", ++gensym_count); };

  // Mapping of instruction addresses to decoded instructions
  Disassembly result;

  // Mapping of instruction addresses to jump target label name
  std::map<int, std::string> label_names;

  // The flag_state when execution reaches a given address
  std::map<int, FlagState> entry_state;
  entry_state[starting_address] = initial_flag_state;

  //
}

}  // namespace nsasm
