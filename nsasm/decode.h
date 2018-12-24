#ifndef NSASM_DECODE_H_
#define NSASM_DECODE_H_

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "nsasm/flag_state.h"
#include "nsasm/instruction.h"

#include <cstdint>

namespace nsasm {

// Returns a 65816 instruction decoded from a chunk of memory.
//
// If a valid instruction can't be found (because there aren't enough bytes to
// read, or because the provided FlagState is uncertain about a processor flag
// required for proper decoding), returns nullopt instead.
absl::optional<Instruction> Decode(absl::Span<uint8_t> bytes,
                                   const FlagState& state);

}  // namespace nsasm

#endif  // NSASM_DECODE_H_
