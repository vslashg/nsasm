#ifndef NSASM_ASSEMBLER_H
#define NSASM_ASSEMBLER_H

#include "absl/container/flat_hash_map.h"
#include "nsasm/error.h"
#include "nsasm/module.h"

namespace nsasm {

class Assembler {
 public:
  nsasm::ErrorOr<void> AddModule(Module&& module);
  nsasm::ErrorOr<void> AddAsmFile(const std::string& path);

 private:
  absl::flat_hash_map<std::string, Module> named_modules_;
  std::deque<Module> unnamed_modules_;
};

}  // namespace nsasm

#endif  // NSASM_ASSEMBLER_H
