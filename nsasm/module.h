#ifndef NSASM_MODULE_H
#define NSASM_MODULE_H

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "nsasm/error.h"
#include "nsasm/statement.h"
#include "nsasm/ranges.h"

namespace nsasm {

class ModuleLookupContext;

class Module {
 public:
  // movable but not copiable
  Module(const Module&) = delete;
  Module(Module&&) = default;

  // Opens the .asm file at the given path, and either returns a Module loaded
  // from it, or an error.
  //
  // path_, module_name_, and dependencies_ are set on the removed module.
  static ErrorOr<Module> LoadAsmFile(const std::string& path);

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
  ErrorOr<int> ValueForName(absl::string_view sv) const;

  ErrorOr<void> Assemble(OutputSink* sink, const LookupContext& lookup_context);

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
  ErrorOr<int> LocalLookup(absl::string_view sv,
                           const std::vector<int>& active_scopes) const;

  friend class nsasm::ModuleLookupContext;

  // A line of code (an instruction or directive) inside an .asm module.
  struct Line {
    Line(Statement statement)
        : statement(std::move(statement)), incoming_state() {}
    Statement statement;
    std::vector<std::string> labels;
    bool reached = false;
    FlagState incoming_state;
    absl::optional<int> address;
    std::vector<int> active_scopes;
    absl::flat_hash_map<std::string, int> scoped_locals;
  };

  std::string path_;
  std::string module_name_;
  std::vector<Line> lines_;
  std::set<std::string> dependencies_;
  absl::flat_hash_map<std::string, int> global_to_line_;

  DataRange owned_bytes_;
  absl::flat_hash_map<int, std::string> value_to_global_;
  std::map<int, FlagState> unnamed_targets_;
};

}  // namespace nsasm

#endif  // NSASM_MODULE_H
