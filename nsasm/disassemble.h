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
  bool is_entry = false;
  FlagState current_flag_state;
  FlagState next_flag_state;
};

using DisassemblyMap = std::map<int, DisassembledInstruction>;

class Disassembler {
 public:
  Disassembler(Rom&& rom) : rom_(rom), status_{}, current_sym_(0) {}

  // movable but not copiable
  Disassembler(const Disassembler&) = delete;
  Disassembler& operator=(const Disassembler&) = delete;
  Disassembler(Disassembler&&) = default;
  Disassembler& operator=(Disassembler&&) = default;

  ErrorOr<void> Disassemble(int starting_address,
                            const FlagState& initial_flag_state);
  ErrorOr<void> Cleanup();

  const DisassemblyMap& Result() const { return disassembly_; }

 private:
  std::string GenSym() { return absl::StrCat("gensym", ++current_sym_); }

  Rom rom_;
  ErrorOr<void> status_;
  std::set<int> entry_points_;
  std::map<int, DisassembledInstruction> disassembly_;
  int current_sym_;
};

};  // namespace nsasm

#endif  // NSASM_DISASSEMBLE_H_
