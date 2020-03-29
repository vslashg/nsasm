#include "nsasm/module.h"

#include <fstream>

#include "absl/strings/str_format.h"
#include "nsasm/parse.h"
#include "nsasm/token.h"

namespace nsasm {

class ModuleLookupContext : public LookupContext {
 public:
  ModuleLookupContext(Module* module, const std::vector<int>& active_scopes,
                      const LookupContext& extern_vars)
      : module_(module), active_scopes_(active_scopes), externs_(extern_vars) {}

  ErrorOr<int> Lookup(const FullIdentifier& id) const override {
    // This will change when lookup/module strategy changes
    if (!id.Qualified() || id.Module() == module_->Name()) {
      auto lv = module_->LocalLookup(id.Identifier(), active_scopes_);
      NSASM_RETURN_IF_ERROR(lv);
      return lv->ToInt();
    } else {
      return externs_.Lookup(id);
    }
  }

 private:
  Module* module_;
  const std::vector<int>& active_scopes_;
  const LookupContext& externs_;
};

ErrorOr<Module> Module::LoadAsmFile(const File& file) {
  Module m;
  m.path_ = file.path();

  Location loc(file.path());

  std::vector<ParsedLabel> pending_labels;
  std::vector<int> active_scopes;

  auto add_label = [&m, &active_scopes](
                       const ParsedLabel& label,
                       int target_line) -> nsasm::ErrorOr<void> {
    absl::flat_hash_map<std::string, int>* scope;
    if (active_scopes.empty() || label.exported) {
      scope = &m.global_to_line_;
    } else {
      scope = &m.lines_[active_scopes.back()].scoped_locals;
    }
    if (scope->contains(label.name)) {
      return Error("Duplicate label definition for '%s'", label.name);
    }
    (*scope)[label.name] = target_line;
    return {};
  };

  int line_number = 0;
  for (const std::string& line : file) {
    ++line_number;
    loc.Update(line_number);
    auto tokens = nsasm::Tokenize(line, loc);
    NSASM_RETURN_IF_ERROR(tokens);
    auto entities = nsasm::Parse(*tokens);
    NSASM_RETURN_IF_ERROR(entities);

    for (auto& entity : *entities) {
      if (auto* label = absl::get_if<ParsedLabel>(&entity)) {
        pending_labels.push_back(std::move(*label));
      } else if (auto* statement = absl::get_if<Statement>(&entity)) {
        m.lines_.emplace_back(std::move(*statement));
        for (const ParsedLabel& pending_label : pending_labels) {
          NSASM_RETURN_IF_ERROR_WITH_LOCATION(
              add_label(pending_label, m.lines_.size() - 1), loc);
          m.lines_.back().labels.push_back(pending_label.name);
        }
        m.lines_.back().active_scopes = active_scopes;
        pending_labels.clear();
        const Directive* directive = m.lines_.back().statement.Directive();
        if (directive) {
          if (directive->name == D_module) {
            if (!m.module_name_.empty()) {
              return Error("Duplicate %s directive", ToString(directive->name))
                  .SetLocation(loc);
            }
            auto id = directive->argument.SimpleIdentifier();
            if (!id) {
              return Error("logic error: %s directive with complex expression",
                           ToString(directive->name))
                  .SetLocation(loc);
            }
            m.module_name_ = *id;
          } else if (directive->name == D_equ) {
            auto dependencies = directive->argument.ModuleNamesReferenced();
            m.dependencies_.insert(dependencies.begin(), dependencies.end());
          } else if (directive->name == D_begin) {
            active_scopes.push_back(m.lines_.size() - 1);
          } else if (directive->name == D_end) {
            if (active_scopes.empty()) {
              return Error("Scope close without matching open")
                  .SetLocation(loc);
            }
            active_scopes.pop_back();
          }
        }
      } else {
        return Error("logic error: unknown entity type");
      }
    }
  }
  if (!active_scopes.empty()) {
    return Error("Scope open without matching close")
        .SetLocation(m.lines_[active_scopes.back()].statement.Location());
  }
  return m;
}

ErrorOr<void> Module::RunFirstPass() {
  // Map of lines to evaluate next, and the flag state on entry
  std::map<size_t, ExecutionState> decode_stack;
  auto add_to_decode_stack = [&decode_stack](size_t line,
                                             const ExecutionState& state) {
    auto it = decode_stack.find(line);
    if (it == decode_stack.end()) {
      decode_stack[line] = state;
    } else {
      it->second |= state;
    }
  };
  // Find all .entry points in the module to begin static analysis.
  for (size_t i = 0; i < lines_.size(); ++i) {
    Line& line = lines_[i];
    if (line.statement == D_entry) {
      add_to_decode_stack(i + 1,
                          line.statement.Directive()->flag_state_argument);
    }
  }

  while (!decode_stack.empty()) {
    // Consider the earliest statement that needs processing.
    auto& node = *decode_stack.begin();
    if (node.first == lines_.size()) {
      return Error("Execution continues past end of file");
    }
    Line& line = lines_[node.first];
    const ExecutionState& current_state = node.second;
    if (line.reached && line.incoming_state == current_state) {
      // we've been here before under these conditions; nothing more to do
      decode_stack.erase(decode_stack.begin());
      continue;
    }

    line.reached = true;
    line.incoming_state = current_state;

    ExecutionState next_state = current_state;
    NSASM_RETURN_IF_ERROR_WITH_LOCATION(line.statement.Execute(&next_state),
                                        line.statement.Location());
    if (!line.statement.IsExitInstruction()) {
      add_to_decode_stack(node.first + 1, next_state);
    }
    if (line.statement.IsLocalBranch()) {
      const Instruction& ins = *line.statement.Instruction();
      auto target = ins.arg1.SimpleIdentifier();
      if (!target) {
        return Error("logic error: branch instruction argument missing?");
      }
      auto target_index = LocalIndex(*target, line.active_scopes);
      if (!target_index.ok()) {
        return Error("Target for `%s %s` not found",
                     nsasm::ToString(ins.mnemonic), *target)
            .SetLocation(ins.location);
      }
      next_state = current_state;
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(ins.ExecuteBranch(&next_state),
                                          line.statement.Location());
      add_to_decode_stack(*target_index, next_state);
    }
  }

  // Check that all lines are reachable, and choose their ultimate addressing
  // modes.
  for (Line& line : lines_) {
    Instruction* ins = line.statement.Instruction();
    if (ins && !line.reached) {
      return Error("Line not reached during execution")
          .SetLocation(ins->location);
    }
    if (ins) {
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(
          ins->FixAddressingMode(line.incoming_state.Flags()), ins->location);
    }
  }

  // Assign an address to each statement.
  absl::optional<nsasm::Address> pc = absl::nullopt;
  for (Line& line : lines_) {
    Directive* dir = line.statement.Directive();
    if (dir && dir->name == D_org) {
      auto v = dir->argument.Evaluate(NullLookupContext());
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(v, dir->location);
      pc = nsasm::Address(*v);
    }
    int statement_size = line.statement.SerializedSize();
    if (statement_size > 0 && !pc.has_value()) {
      return Error("No address given for assembly")
          .SetLocation(line.statement.Location());
    }
    if (pc.has_value()) {
      if (!dir || dir->name != D_equ) {
        line.value = LabelValue(*pc);
      }
      pc = pc->AddWrapped(statement_size);
    }
  }

  return {};
}

ErrorOr<int> Module::LocalIndex(absl::string_view sv,
                                const std::vector<int>& active_scopes) const {
  std::vector<const absl::flat_hash_map<std::string, int>*> scopes;
  for (auto it = active_scopes.rbegin(); it != active_scopes.rend(); ++it) {
    scopes.push_back(&lines_[*it].scoped_locals);
  }
  scopes.push_back(&global_to_line_);
  for (auto scope : scopes) {
    auto line_it = scope->find(sv);
    if (line_it == scope->end()) {
      continue;
    }
    return line_it->second;
  }
  return Error("Reference to undefined name '%s'", sv);
}

ErrorOr<LabelValue> Module::LocalLookup(
    absl::string_view sv, const std::vector<int>& active_scopes) const {
  auto index = LocalIndex(sv, active_scopes);
  NSASM_RETURN_IF_ERROR(index);
  const Line& line = lines_[*index];
  if (line.value.has_value()) {
    return LabelValue::FromInt(line.value->ToInt());
  }
  return Error("Value '%s' accessed before definition", sv);
}

ErrorOr<void> Module::RunSecondPass(const LookupContext& lookup_context) {
  // Second pass is for evaluating .equ expressions only.
  for (Line& line : lines_) {
    const Directive* dir = line.statement.Directive();
    if (dir && dir->name == D_equ && !line.value.has_value()) {
      ModuleLookupContext context(this, line.active_scopes, lookup_context);
      auto value = dir->argument.Evaluate(context);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(value, line.statement.Location());
      line.value = LabelValue::FromInt(*value);
    }
  }
  return {};
}

ErrorOr<void> Module::Assemble(OutputSink* sink,
                               const LookupContext& lookup_context) {
  for (Line& line : lines_) {
    auto* directive = line.statement.Directive();
    auto* instruction = line.statement.Instruction();
    const int size = line.statement.SerializedSize();
    ModuleLookupContext context(this, line.active_scopes, lookup_context);
    if (size > 0) {
      if (!line.value.has_value()) {
        return Error("logic error: no address for statement")
            .SetLocation(line.statement.Location());
      }
      Address address = line.value->ToAddress();
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(
          line.statement.Assemble(address, context, sink),
          line.statement.Location());
      if (!owned_bytes_.ClaimBytes(address, size)) {
        return Error("Second write to same address %s in module",
                     address.ToString())
            .SetLocation(line.statement.Location());
      }
    } else if (directive && directive->name == D_entry) {
      if (!line.value.has_value()) {
        return Error("logic error: no address for .entry directive")
            .SetLocation(line.statement.Location());
      }
      Address address = line.value->ToAddress();
      if (!directive->return_convention_argument.IsDefault()) {
        return_conventions_[address] = directive->return_convention_argument;
      }
    } else if (directive && directive->name == D_remote) {
      auto arg_val = directive->argument.Evaluate(context);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(arg_val, directive->location);
      Address address(*arg_val);
      auto it = unnamed_targets_.find(address);
      if (it == unnamed_targets_.end()) {
        unnamed_targets_[address] = directive->flag_state_argument;
      } else {
        it->second |= directive->flag_state_argument;
      }
      if (!directive->return_convention_argument.IsDefault()) {
        return_conventions_[address] = directive->return_convention_argument;
      }
    }
    if (instruction) {
      // We know line.value has a value, or we wouldn't have gotten this far
      auto branch_target =
          instruction->FarBranchTarget(line.value->ToAddress());
      if (branch_target.has_value()) {
        // FarBranchTarget() does not perform lookup, so if we have a value,
        // this is our branch target.
        auto it = unnamed_targets_.find(*branch_target);
        if (it == unnamed_targets_.end()) {
          unnamed_targets_[*branch_target] = line.incoming_state.Flags();
        } else {
          it->second |= line.incoming_state.Flags();
        }
      }
    }
  }
  for (const auto& node : global_to_line_) {
    const Line& line = lines_[node.second];
    if (!line.value.has_value()) {
      return Error("Label missing a value")
          .SetLocation(line.statement.Location());
    }
    nsasm::Address address = line.value->ToAddress();
    if (!address_to_global_.contains(address)) {
      address_to_global_[address] = node.first;
    }
  }
  return {};
}

void Module::DebugPrint() const {
  for (const auto& line : lines_) {
    for (const auto& label : line.labels) {
      absl::PrintF("       %s:\n", label);
    }
    if (line.value.has_value()) {
      absl::PrintF("%06x     %s\n", line.value->ToNumber(T_long),
                   line.statement.ToString());
    } else {
      absl::PrintF("           %s\n", line.statement.ToString());
    }
  }
}

ErrorOr<LabelValue> Module::ValueForName(absl::string_view sv) const {
  auto line_loc = global_to_line_.find(sv);
  if (line_loc == global_to_line_.end()) {
    return Error("No label named %s in module %s", sv, module_name_);
  }
  const absl::optional<LabelValue>& value = lines_[line_loc->second].value;
  if (value.has_value()) {
    return *value;
  }
  return Error("logic error: No value at label %s::%s", module_name_, sv);
}

absl::optional<std::string> Module::NameForAddress(
    nsasm::Address address) const {
  if (module_name_.empty()) {
    return absl::nullopt;
  }
  auto it = address_to_global_.find(address);
  if (it == address_to_global_.end()) {
    return absl::nullopt;
  }
  return absl::StrCat(module_name_, "::", it->second);
}

}  // namespace nsasm
