#ifndef NSASM_ASSEMBLER_H
#define NSASM_ASSEMBLER_H

#include "absl/container/flat_hash_map.h"
#include "nsasm/error.h"
#include "nsasm/module.h"
#include "nsasm/ranges.h"

namespace nsasm {

class AssemblerLookupContext;

class Assembler {
 public:
  ErrorOr<void> AddModule(Module&& module);
  ErrorOr<void> AddAsmFile(const std::string& path);

  // Assemble all modules together into a single sink.
  ErrorOr<void> Assemble(OutputSink* sink);


  // Post-assembly queries

  // Returns true if data has been assembled into the given byte address.
  bool Contains(int address) const {
    return memory_module_map_.Contains(address);
  }

  // Returns a qualified name for a label referring to this address
  absl::optional<std::string> NameForAddress(int address) const;

  // Returns the collection of all jump targets found during assembly.
  std::map<int, FlagState> JumpTargets() const;

  // Returns the set of jump targets found which yield alternate flag states
  std::map<int, FlagState> JumpTargetYields() const;

  // Output each named module's contents to stdout
  void DebugPrint() const;

 private:
  // Calculates an order of named module assembly so that all
  // .equ expressions are evaluated before any are accessed.
  ErrorOr<std::vector<std::string>> FindAssemblyOrder();

  friend class AssemblerLookupContext;

  absl::flat_hash_map<std::string, std::unique_ptr<Module>> named_modules_;
  std::deque<Module> unnamed_modules_;

  // malevolent murder maze
  RangeMap<Module*> memory_module_map_;
};

}  // namespace nsasm

#endif  // NSASM_ASSEMBLER_H
