#ifndef NSASM_MODULE_H
#define NSASM_MODULE_H

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "nsasm/address.h"
#include "nsasm/error.h"
#include "nsasm/file.h"
#include "nsasm/ranges.h"
#include "nsasm/statement.h"

namespace nsasm {

class ModuleLookupContext;

class Module {
 public:
  // movable but not copiable
  Module(const Module&) = delete;
  Module(Module&&) = default;

  // Takes a given File, and either returns the Module parsed from it, or an
  // error.
  static ErrorOr<Module> LoadAsmFile(const File& file);

  std::string Name() const { return module_name_; }

  // Returns the set of module names that this module depends on.  (This is
  // not every reference, but only for references that require early evaluation,
  // i.e., .EQU arguments.)
  const std::set<std::string> Dependencies() const { return dependencies_; }

  // Run the first pass of this module.  This determines the size of each
  // instruction and assigns an address to each non-.equ label, but will not
  // evaluate any expressions.
  ErrorOr<void> RunFirstPass();

  // Run the .equ evaluation pass.  This determines the value of each .equ
  // expression.  Evaluation of other expressions in the module aren't performed
  // this pass.
  ErrorOr<void> RunSecondPass(const LookupContext& lookup_context);

  // Returns the value for the given name, or nullopt if that name is not
  // defined by this module.  Call this only after RunFirstPass() has
  // successfully returned.
  ErrorOr<LabelValue> ValueForName(absl::string_view sv) const;

  ErrorOr<void> Assemble(OutputSink* sink, const LookupContext& lookup_context);

  // Post-assembly queries

  // Returns a qualified name for the given address, if one is defined in this
  // module.
  absl::optional<std::string> NameForAddress(nsasm::Address address) const;

  // Returns the collection of all jump targets found during assembly.
  const std::map<nsasm::Address, StatusFlags>& JumpTargets() const {
    return unnamed_targets_;
  }

  // Returns the set of jump targets assembled which yield alternate flag states
  const std::map<nsasm::Address, ReturnConvention>&
  JumpTargetReturnConventions() const {
    return return_conventions_;
  }

  const DataRange& OwnedBytes() const { return owned_bytes_; }

  // Output this module's contents to stdout
  void DebugPrint() const;

 private:
  Module() = default;

  // Perform an internal lookup for a given label.  Returns an error if the
  // name does not exist.  Otherwise returns the index into lines_ where this
  // label points.
  ErrorOr<int> LocalIndex(absl::string_view sv,
                          const std::vector<int>& active_scopes) const;

  // Perform an internal lookup for a given label.  As above, but returns the
  // value associated with the label, rather than an index.
  ErrorOr<LabelValue> LocalLookup(absl::string_view sv,
                                  const std::vector<int>& active_scopes) const;

  friend class nsasm::ModuleLookupContext;

  // A line of code (an instruction or directive) inside an .asm module.
  struct Line {
    Line(Statement statement)
        : statement(std::move(statement)), incoming_state() {}
    Statement statement;
    std::vector<std::string> labels;
    bool reached = false;
    ExecutionState incoming_state;
    absl::optional<LabelValue> value;
    std::vector<int> active_scopes;
    absl::flat_hash_map<std::string, int> scoped_locals;
  };

  std::string path_;
  std::string module_name_;
  std::vector<Line> lines_;
  std::set<std::string> dependencies_;
  absl::flat_hash_map<std::string, int> global_to_line_;

  DataRange owned_bytes_;
  absl::flat_hash_map<nsasm::Address, std::string> address_to_global_;
  std::map<nsasm::Address, StatusFlags> unnamed_targets_;
  std::map<nsasm::Address, ReturnConvention> return_conventions_;
};

}  // namespace nsasm

#endif  // NSASM_MODULE_H
