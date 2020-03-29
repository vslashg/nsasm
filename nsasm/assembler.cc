#include "nsasm/assembler.h"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_format.h"

namespace nsasm {

void Assembler::AddModule(Module&& module) {
  modules_.push_back(std::move(module));
}

nsasm::ErrorOr<void> Assembler::AddAsmFile(const File& file) {
  auto module = Module::LoadAsmFile(file);
  NSASM_RETURN_IF_ERROR(module);
  AddModule(*std::move(module));
  return {};
}

nsasm::ErrorOr<std::vector<Module*>> Assembler::FindAssemblyOrder() {
  std::vector<Module*> order;

  absl::flat_hash_set<Module*> already_inserted;
  absl::flat_hash_map<FullIdentifier, Location> exported_names;

  while (order.size() < modules_.size()) {
    bool more_found = false;
    for (auto& module : modules_) {
      if (already_inserted.contains(&module)) {
        continue;
      }
      bool all_dependencies_met = true;
      for (const FullIdentifier& dependency : module.Dependencies()) {
        if (!exported_names.contains(dependency)) {
          all_dependencies_met = false;
          break;
        }
      }
      if (all_dependencies_met) {
        more_found = true;
        order.push_back(&module);
        already_inserted.insert(&module);
        for (const auto& node : module.ExportedNames()) {
          if (exported_names.contains(node.first)) {
            return Error(
                       "Duplicate defintion of `%s` "
                       "(conflicting definition at %s)",
                       node.first.ToString(), node.second.ToString())
                .SetLocation(exported_names[node.first]);
          }
          exported_names[node.first] = node.second;
          name_to_module_map_[node.first] = &module;
        }
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

  ErrorOr<int> Lookup(const FullIdentifier& id) const override {
    if (!id.Qualified()) {
      return Error("logic error: assembler lookup passed unqualified name '%s'",
                   id.ToString());
    }
    auto it = assembler_->name_to_module_map_.find(id);
    if (it == assembler_->name_to_module_map_.end()) {
      return Error("No definition for '%s' found", id.ToString());
    }
    auto v = it->second->ValueForName(id);
    NSASM_RETURN_IF_ERROR(v);
    return v->ToInt();
  }

 private:
  const Assembler* assembler_;
};

nsasm::ErrorOr<void> Assembler::Assemble(OutputSink* sink) {
  auto module_order = FindAssemblyOrder();
  NSASM_RETURN_IF_ERROR(module_order);

  // First pass: laying out code and finding the address of each instruction.
  for (Module* module : *module_order) {
    NSASM_RETURN_IF_ERROR(module->RunFirstPass());
  }

  AssemblerLookupContext context(this);

  // Second pass: evaluating .equ expressions.
  for (Module* module : *module_order) {
    NSASM_RETURN_IF_ERROR(module->RunSecondPass(context));
  }

  // Final pass: assembly
  for (Module* module : *module_order) {
    NSASM_RETURN_IF_ERROR(module->Assemble(sink, context));
    if (!memory_module_map_.Insert(module->OwnedBytes(), module)) {
      // TODO: this could convey a lot more info...
      return nsasm::Error("Module `%s` writing to previously claimed memory",
                          module->Name());
    }
  }

  return {};
}

absl::optional<FullIdentifier> Assembler::NameForAddress(
    nsasm::Address address) const {
  for (const Module& module : modules_) {
    auto v = module.NameForAddress(address);
    if (v.has_value()) {
      return v;
    }
  }
  return absl::nullopt;
}

std::map<nsasm::Address, StatusFlags> Assembler::JumpTargets() const {
  // TODO: add support for warnings, and report on jumps into existing modules

  std::map<nsasm::Address, StatusFlags> ret;
  for (const Module& module : modules_) {
    for (auto& node : module.JumpTargets()) {
      nsasm::Address dest = node.first;
      if (!Contains(dest)) {
        auto it = ret.find(node.first);
        if (it == ret.end()) {
          ret[node.first] = node.second;
        } else {
          it->second |= node.second;
        }
      }
    }
  }
  return ret;
}

std::map<nsasm::Address, ReturnConvention>
Assembler::JumpTargetReturnConventions() const {
  std::map<nsasm::Address, ReturnConvention> ret;
  for (const Module& m : modules_) {
    const std::map<nsasm::Address, ReturnConvention>& yields =
        m.JumpTargetReturnConventions();
    ret.insert(yields.begin(), yields.end());
  }
  return ret;
}

void Assembler::DebugPrint() const {
  for (const Module& module : modules_) {
    absl::PrintF("  === debug info for %s\n", module.Path());
    module.DebugPrint();
  }
}

ErrorOr<Assembler> Assemble(const std::vector<File>& files, OutputSink* sink) {
  Assembler a;
  for (const File& file : files) {
    NSASM_RETURN_IF_ERROR(a.AddAsmFile(file));
  }
  NSASM_RETURN_IF_ERROR(a.Assemble(sink));
  return a;
}

}  // namespace nsasm
