#ifndef NSASM_OPCODE_MAP_H_
#define NSASM_OPCODE_MAP_H_

#include <algorithm>
#include <cstdint>

#include "absl/types/optional.h"
#include "nsasm/addressing_mode.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

std::pair<Mnemonic, AddressingMode> DecodeOpcode(uint8_t opcode);

absl::optional<std::uint8_t> EncodeOpcode(Mnemonic m, AddressingMode a);

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

// Returns true iff this mnemonic and addressing mode pair is valid.
bool IsLegalCombination(Mnemonic m, AddressingMode a);

}  // namespace nsasm

#endif  // NSASM_OPCODE_MAP_H_
