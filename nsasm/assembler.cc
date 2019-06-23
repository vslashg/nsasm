#include "nsasm/assembler.h"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_format.h"

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
    named_modules_[name] = absl::make_unique<Module>(std::move(module));
  }
  return {};
}

nsasm::ErrorOr<void> Assembler::AddAsmFile(const std::string& path) {
  auto module = Module::LoadAsmFile(path);
  NSASM_RETURN_IF_ERROR(module);
  return AddModule(*std::move(module));
}

nsasm::ErrorOr<std::vector<std::string>> Assembler::FindAssemblyOrder() {
  std::vector<std::string> order;

  absl::flat_hash_set<std::string> already_inserted;
  while (order.size() < named_modules_.size()) {
    bool more_found = false;
    for (const auto& module : named_modules_) {
      if (already_inserted.contains(module.first)) {
        continue;
      }
      bool all_dependencies_met = true;
      for (const std::string& dependency : module.second->Dependencies()) {
        if (!already_inserted.contains(dependency)) {
          all_dependencies_met = false;
          break;
        }
      }
      if (all_dependencies_met) {
        more_found = true;
        order.push_back(module.first);
        already_inserted.insert(module.first);
      }
    }
    if (!more_found) {
      // TODO: Return an error message explaining the cyclic dependency
      return Error("Cyclic dependency in .equ definitions");
    }
  }
  return order;
}

class AssemblerLookupContext : public LookupContext {
 public:
  AssemblerLookupContext(const Assembler* assembler) : assembler_(assembler) {}

  ErrorOr<int> Lookup(absl::string_view name,
                      absl::string_view module) const override {
    auto module_it = assembler_->named_modules_.find(module);
    if (module_it == assembler_->named_modules_.end()) {
      return Error("No such module '%s' (resolving '%s::%s')", module, module,
                   name);
    }
    return module_it->second->ValueForName(name);
  }

 private:
  const Assembler* assembler_;
};

nsasm::ErrorOr<void> Assembler::Assemble(OutputSink* sink) {
  auto module_name_order = FindAssemblyOrder();
  NSASM_RETURN_IF_ERROR(module_name_order);

  // Get the order that modules are to be assembled in each pass
  std::vector<Module*> module_order;
  for (const std::string& name : *module_name_order) {
    auto module_iter = named_modules_.find(name);
    module_order.push_back(module_iter->second.get());
  }
  for (Module& module : unnamed_modules_) {
    module_order.push_back(&module);
  }

  // First pass: laying out code and finding the address of each instruction.
  for (Module* module : module_order) {
    NSASM_RETURN_IF_ERROR(module->RunFirstPass());
  }

  AssemblerLookupContext context(this);

  // Second pass: evaluating .equ expressions.
  for (Module* module : module_order) {
    NSASM_RETURN_IF_ERROR(module->RunSecondPass(context));
  }

  // Final pass: assembly
  for (Module* module : module_order) {
    NSASM_RETURN_IF_ERROR(module->Assemble(sink, context));
    if (!memory_module_map_.Insert(module->OwnedBytes(), module)) {
      // TODO: this could convey a lot more info...
      return nsasm::Error("Module `%s` writing to previously claimed memory",
                          module->Name());
    }
  }

  return {};
}

absl::optional<std::string> Assembler::NameForAddress(int address) const {
  for (const auto& node : named_modules_) {
    auto v = node.second->NameForAddress(address);
    if (v.has_value()) {
      return v;
    }
  }
  return absl::nullopt;
}

void Assembler::DebugPrint() const {
  for (const auto& node : named_modules_) {
    absl::PrintF("  === debug info for %s\n", node.second->Name());
    node.second->DebugPrint();
  }
}

}  // namespace nsasm
