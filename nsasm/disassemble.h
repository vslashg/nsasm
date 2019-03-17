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
  FlagState current_flag_state;
  FlagState next_flag_state;
};

using Disassembly = std::map<int, DisassembledInstruction>;

ErrorOr<Disassembly> Disassemble(const Rom& rom,
                                 const std::map<int, FlagState>& seed_map);

inline ErrorOr<Disassembly> Disassemble(const Rom& rom, int starting_address,
                                        const FlagState& initial_flag_state) {
  std::map<int, FlagState> seed_map = {{starting_address, initial_flag_state}};
  return Disassemble(rom, seed_map);
}
};  // namespace nsasm

#endif  // NSASM_DISASSEMBLE_H_
