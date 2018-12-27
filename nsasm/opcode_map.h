#ifndef NSASM_OPCODE_MAP_H_
#define NSASM_OPCODE_MAP_H_

#include <cstdint>

#include "nsasm/flag_state.h"
#include "nsasm/instruction.h"

namespace nsasm {

Instruction DecodeOpcode(uint8_t opcode);

// Returns true if the given instruction is consistent with the provided flag
// state.
bool IsConsistent(const Instruction& instruction, const FlagState& flag_state);

}  // namespace nsasm

#endif  // NSASM_OPCODE_MAP_H_