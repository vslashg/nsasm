#ifndef NSASM_DISASSEMBLE_H_
#define NSASM_DISASSEMBLE_H_

#include <map>
#include <string>

#include "nsasm/error.h"
#include "nsasm/flag_state.h"
#include "nsasm/instruction.h"
#include "nsasm/rom.h"

namespace nsasm {

struct DisassembledInstruction {
  std::string label;
  Instruction instruction;
};

using Disassembly = std::map<int, DisassembledInstruction>;

ErrorOr<Disassembly> Disassemble(const Rom& rom, int starting_address,
                                 const FlagState& initial_flag_state);

};

#endif  // NSASM_DISASSEMBLE_H_
