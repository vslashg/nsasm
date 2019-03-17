#ifndef NSASM_MODULE_H
#define NSASM_MODULE_H

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "nsasm/error.h"
#include "nsasm/statement.h"

namespace nsasm {

class Module {
 public:
  // movable but not copiable
  Module(const Module&) = delete;
  Module(Module&&) = default;

  // Opens the .asm file at the given path, and either returns a Module loaded
  // from it, or an error.
  static ErrorOr<Module> LoadAsmFile(const std::string& path);

  std::string ModuleName() const { return module_name_; }

  // Returns the set of module names that this module depends on.  (This is
  // not every reference, but only for references that require early evaluation,
  // i.e., .EQU arguments.)
  const std::set<std::string> Dependencies() const { return dependencies_; }

  ErrorOr<void> RunFirstPass();

  ErrorOr<void> Assemble(OutputSink* sink);

  // Output this module's contents to stdout
  void DebugPrint() const;

 private:
  Module() = default;

  // A line of code (an instruction or directive) inside an .asm module.
  struct Line {
    Line(Statement statement)
        : statement(std::move(statement)), incoming_state() {}
    Statement statement;
    std::vector<std::string> labels;
    bool reached = false;
    FlagState incoming_state;
    int address = -1;
  };

  std::string path_;
  std::string module_name_;
  std::vector<Line> lines_;
  std::set<std::string> dependencies_;
  absl::flat_hash_map<std::string, int> label_map_;
};

}  // namespace nsasm

#endif  // NSASM_MODULE_H
