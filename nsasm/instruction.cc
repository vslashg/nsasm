#include "nsasm/instruction.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string Instruction::ToString() const {
  return absl::StrFormat("%s%s", nsasm::ToString(mnemonic),
                         ArgsToString(addressing_mode, arg1, arg2));
}

}  // namespace nsasm
