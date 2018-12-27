#ifndef NSASM_DISASSEMBLE_H_
#define NSASM_DISASSEMBLE_H_

#include <map>
#include <string>

#include "nsasm/flag_state.h"
#include "nsasm/instruction.h"
#include "nsasm/rom.h"

namespace nsasm {

struct DisassembledInstruction {
  std::string label;
  Instruction instruction;
};

using Disassembly = std::map<int, DisassembledInstruction>;

absl::optional<Disassembly> Disassemble(const Rom& rom, int starting_address,
                                        const FlagState& initial_flag_state);

};

#endif  // NSASM_DISASSEMBLE_H_
