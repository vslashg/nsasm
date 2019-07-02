#ifndef NSASM_DISASSEMBLE_H_
#define NSASM_DISASSEMBLE_H_

#include <map>
#include <string>

#include "absl/types/optional.h"
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
  Disassembler(Rom&& rom) : rom_(rom), current_sym_(0) {}

  // movable but not copiable
  Disassembler(const Disassembler&) = delete;
  Disassembler& operator=(const Disassembler&) = delete;
  Disassembler(Disassembler&&) = default;
  Disassembler& operator=(Disassembler&&) = default;

  // Disassemble code starting at the given address and state, and store
  // the results in the internal disassembly map.
  //
  // Returns an error, or else a mapping of all far jump targets found in
  // this disassembly.
  ErrorOr<std::map<int, FlagState>> Disassemble(
      int starting_address, const FlagState& initial_flag_state);
  ErrorOr<void> Cleanup();

  const DisassemblyMap& Result() const { return disassembly_; }

  // Install a set of subroutine return calling convention.  Any subroutine jump
  // to the given address is disassembled with the given return convention.
  // This will cause jumps to the provided addresses to set the flag state, or
  // even stop further execution.
  void AddTargetReturnConventions(
      const std::map<int, ReturnConvention>& return_conventions) {
    return_conventions_ = return_conventions;
  }

 private:
  absl::optional<std::string> NameForAddress(int address);

  std::string GenSym() { return absl::StrCat("gensym", ++current_sym_); }

  Rom rom_;
  std::set<int> entry_points_;
  std::map<int, DisassembledInstruction> disassembly_;
  std::map<int, ReturnConvention> return_conventions_;
  int current_sym_;
};

};  // namespace nsasm

#endif  // NSASM_DISASSEMBLE_H_
