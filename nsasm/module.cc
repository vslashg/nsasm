#include "nsasm/module.h"

#include <fstream>

#include "nsasm/parse.h"
#include "nsasm/token.h"

namespace nsasm {

ErrorOr<Module> nsasm::Module::LoadAsmFile(const std::string& path) {
  std::ifstream fs(path);
  if (!fs.good()) {
    return Error("Unable to open file %s", path);
  }
  Module m;

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
          m.label_map_[pending_label] = m.lines_.size();
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

void nsasm::Module::DebugPrint() const {
  for (const auto& line : lines_) {
    for (const auto& label : line.labels) {
      absl::PrintF("%s:\n", label);
    }
    absl::PrintF("    %s\n", line.statement.ToString());
  }
}

}  // namespace
