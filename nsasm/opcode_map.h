#ifndef NSASM_OPCODE_MAP_H_
#define NSASM_OPCODE_MAP_H_

#include <cstdint>

#include "nsasm/addressing_mode.h"
#include "nsasm/mnemonic.h"
#include "nsasm/flag_state.h"

namespace nsasm {

struct DecodedOp {
  Mnemonic mnemonic;
  AddressingMode mode;
};

DecodedOp DecodeOpcode(uint8_t opcode);

}  // namespace nsasm

#endif  // NSASM_OPCODE_MAP_H_