#ifndef NSASM_ASSEMBLER_H
#define NSASM_ASSEMBLER_H

#include "absl/container/flat_hash_map.h"
#include "nsasm/error.h"
#include "nsasm/module.h"

namespace nsasm {

class AssemblerLookupContext;

class Assembler {
 public:
  ErrorOr<void> AddModule(Module&& module);
  ErrorOr<void> AddAsmFile(const std::string& path);

  // Assemble all modules together into a single sink.
  ErrorOr<void> Assemble(OutputSink* sink);

  // Output each named module's contents to stdout
  void DebugPrint() const;

 private:
  // Calculates an order of named module assembly so that all
  // .equ expressions are evaluated before any are accessed.
  ErrorOr<std::vector<std::string>> FindAssemblyOrder();

  friend class AssemblerLookupContext;

  absl::flat_hash_map<std::string, Module> named_modules_;
  std::deque<Module> unnamed_modules_;
};

}  // namespace nsasm

#endif  // NSASM_ASSEMBLER_H
