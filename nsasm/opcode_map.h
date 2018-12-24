#ifndef NSASM_OPCODE_MAP_H_
#define NSASM_OPCODE_MAP_H_

#include <cstdint>

#include "nsasm/instruction.h"

namespace nsasm {

Instruction DecodeOpcode(uint8_t opcode);

}  // namespace nsasm

#endif  // NSASM_OPCODE_MAP_H_