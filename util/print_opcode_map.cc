#include "absl/strings/str_format.h"
#include "nsasm/expression.h"
#include "nsasm/instruction.h"
#include "nsasm/opcode_map.h"

int main() {
  for (int i = 0; i < 256; ++i) {
    nsasm::Instruction op = nsasm::DecodeOpcode(i);
    std::string suffix = "";
    if (op.addressing_mode == nsasm::A_imm_fm ||
        op.addressing_mode == nsasm::A_imm_fx) {
      suffix = ".b";
      op.addressing_mode = nsasm::A_imm_b;
    }
    absl::PrintF(
        "%02X  %s%s%s\n", i, nsasm::ToString(op.mnemonic), suffix,
        nsasm::ArgsToString(op.addressing_mode, nsasm::Literal(0x654321),
                            nsasm::Literal(0x43)));
  }
  return 0;
}
