#include "nsasm/assembler.h"

namespace nsasm {

nsasm::ErrorOr<void> Assembler::AddModule(Module&& module) {
  if (module.Name().empty()) {
    unnamed_modules_.push_back(std::move(module));
  } else {
    std::string name = module.Name();
    if (named_modules_.contains(name)) {
      return nsasm::Error("Multiple files have the same module name \"%s\"",
                          name);
    }
    named_modules_.emplace(name, std::move(module));
  }
  return {};
}

nsasm::ErrorOr<void> Assembler::AddAsmFile(const std::string& path) {
  auto module = Module::LoadAsmFile(path);
  NSASM_RETURN_IF_ERROR(module);
  return AddModule(*std::move(module));
}

}  // namespace nsasm
