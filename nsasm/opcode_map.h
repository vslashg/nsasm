#ifndef NSASM_OPCODE_MAP_H_
#define NSASM_OPCODE_MAP_H_

#include <cstdint>

#include "nsasm/flag_state.h"
#include "nsasm/instruction.h"

namespace nsasm {

Instruction DecodeOpcode(uint8_t opcode);

// Returns true iff the given mnemonic takes an immediate argument whose size is
// controlled by the M status bit.
bool ImmediateArgumentUsesMBit(Mnemonic m);

// Returns true iff the given mnemonic takes an immediate argument whose size is
// controlled by the X status bit.
bool ImmediateArgumentUsesXBit(Mnemonic m);

// Returns true iff this mnemonic takes an offset (that is, if it is a branch
// instruction.)
bool TakesOffsetArgument(Mnemonic m);

// Returns true iff this mnemonic takes a 16-bit offset.
bool TakesLongOffsetArgument(Mnemonic m);

// Returns true if the given instruction is consistent with the provided flag
// state.
bool IsConsistent(const Instruction& instruction, const FlagState& flag_state);

}  // namespace nsasm

#endif  // NSASM_OPCODE_MAP_H_