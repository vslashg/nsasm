#ifndef NSASM_ASSEMBLER_H
#define NSASM_ASSEMBLER_H

#include "absl/container/flat_hash_map.h"
#include "nsasm/calling_convention.h"
#include "nsasm/error.h"
#include "nsasm/module.h"
#include "nsasm/ranges.h"

namespace nsasm {

class AssemblerLookupContext;

class Assembler {
 public:
  Assembler(const Assembler&) = delete;
  Assembler& operator=(const Assembler&) = delete;
  Assembler(Assembler&&) = default;
  Assembler& operator=(Assembler&&) = default;

  Assembler() = default;

  ErrorOr<void> AddAsmFile(const File& file);

  // Assemble all modules together into a single sink.
  ErrorOr<void> Assemble(OutputSink* sink);

  // Post-assembly queries

  // Returns true if data has been assembled into the given byte address.
  bool Contains(nsasm::Address address) const {
    return memory_module_map_.Contains(address);
  }

  // Returns a qualified name for a label referring to this address
  absl::optional<std::string> NameForAddress(nsasm::Address address) const;

  // Returns the collection of all jump targets found during assembly.
  std::map<nsasm::Address, StatusFlags> JumpTargets() const;

  // Returns the set of jump targets found with nonstandard calling conventions.
  std::map<nsasm::Address, ReturnConvention> JumpTargetReturnConventions()
      const;

  // Output each named module's contents to stdout
  void DebugPrint() const;

 private:
  ErrorOr<void> AddModule(Module&& module);

  // Calculates an order of named module assembly so that all
  // .equ expressions are evaluated before any are accessed.
  ErrorOr<std::vector<std::string>> FindAssemblyOrder();

  friend class AssemblerLookupContext;

  absl::flat_hash_map<std::string, std::unique_ptr<Module>> named_modules_;
  std::deque<Module> unnamed_modules_;

  // malevolent murder maze
  RangeMap<Module*> memory_module_map_;
};

// Simple factory function for assembling a collection of files
ErrorOr<Assembler> Assemble(const std::vector<File>& files, OutputSink* sink);

}  // namespace nsasm

#endif  // NSASM_ASSEMBLER_H
