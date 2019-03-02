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

  ErrorOr<void> RunFirstPass();

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
  std::vector<Line> lines_;
  absl::flat_hash_map<std::string, int> label_map_;
};

}  // namespace nsasm

#endif  // NSASM_MODULE_H
