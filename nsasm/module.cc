#include "nsasm/module.h"

#include <fstream>

#include "nsasm/parse.h"
#include "nsasm/token.h"

#include "absl/strings/str_format.h"

namespace nsasm {

ErrorOr<Module> Module::LoadAsmFile(const std::string& path) {
  std::ifstream fs(path);
  if (!fs.good()) {
    return Error("Unable to open file %s", path);
  }
  Module m;
  m.path_ = path;

  Location loc;
  loc.path = path;
  loc.offset = 0;

  std::string line;

  std::vector<std::string> pending_labels;
  while (std::getline(fs, line)) {
    ++loc.offset;
    auto tokens = nsasm::Tokenize(line, loc);
    NSASM_RETURN_IF_ERROR(tokens);
    auto entities = nsasm::Parse(*tokens);
    NSASM_RETURN_IF_ERROR(entities);

    for (auto& entity : *entities) {
      if (auto* label = absl::get_if<std::string>(&entity)) {
        pending_labels.push_back(std::move(*label));
      } else if (auto* statement = absl::get_if<Statement>(&entity)) {
        m.lines_.emplace_back(std::move(*statement));
        for (const std::string& pending_label : pending_labels) {
          if (m.label_map_.contains(pending_label)) {
            return Error("Duplicate label definition for '%s'", pending_label)
                .SetLocation(loc);
          }
          m.label_map_[pending_label] = m.lines_.size() - 1;
        }
        m.lines_.back().labels = std::move(pending_labels);
        pending_labels.clear();
      } else {
        return Error("logic error: unknown entity type");
      }
    }
  }
  return m;
}

// Note: we will probably have to one day change this to run over sub-ranges
// of lines_, to support local labels in subroutines, etc.
ErrorOr<void> Module::RunFirstPass() {
  // Map of lines to evaluate next, and the flag state on entry
  std::map<int, FlagState> decode_stack;
  auto add_to_decode_stack = [&decode_stack](int line,
                                             const FlagState& state) {
    auto it = decode_stack.find(line);
    if (it == decode_stack.end()) {
      decode_stack[line] = state;
    } else {
      it->second |= state;
    }
  };

  // Find all .entry points in the module to begin static analysis.
  for (int i = 0; i < lines_.size(); ++i) {
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
    const FlagState& current_state = node.second;
    if (line.reached && line.incoming_state == current_state) {
      // we've been here before under these conditions; nothing more to do
      decode_stack.erase(decode_stack.begin());
      continue;
    }

    line.reached = true;
    line.incoming_state = current_state;

    auto next_state = line.statement.Execute(current_state);
    NSASM_RETURN_IF_ERROR_WITH_LOCATION(next_state, line.statement.Location());
    if (!line.statement.IsExitInstruction()) {
      add_to_decode_stack(node.first + 1, *next_state);
    }
    if (line.statement.IsLocalBranch()) {
      const Instruction& ins = *line.statement.Instruction();
      auto target = ins.arg1.SimpleIdentifier();
      if (!target) {
        return Error("logic error: branch instruction argument missing?");
      }
      auto dest_iter = label_map_.find(*target);
      if (dest_iter == label_map_.end()) {
        return Error("Target for local %s branch not found",
                     nsasm::ToString(ins.mnemonic))
            .SetLocation(ins.location);
      }
      auto branch_state = ins.ExecuteBranch(current_state);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(branch_state, ins.location);
      add_to_decode_stack(dest_iter->second, *branch_state);
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
          ins->FixAddressingMode(line.incoming_state), ins->location);
    }
  }

  return {};
}

void Module::DebugPrint() const {
  for (const auto& line : lines_) {
    for (const auto& label : line.labels) {
      absl::PrintF("%s:\n", label);
    }
    absl::PrintF("    %s\n", line.statement.ToString());
  }
}

}  // namespace
