#include "nsasm/file.h"

#include "absl/strings/ascii.h"
#include "absl/strings/str_split.h"

namespace nsasm {

nsasm::ErrorOr<File> OpenFile(const std::string& path) {
  std::vector<std::string> lines;
  std::ifstream fs(path);
  if (!fs.good()) {
    return Error("Unable to open file %s", path);
  }
  std::string line;
  while (std::getline(fs, line)) {
    lines.emplace_back(std::move(line));
  }
  if (fs.bad()) {
    return Error("Error reading file %s", path);
  }
  return File(path, std::move(lines));
}

nsasm::File MakeFakeFile(const std::string& path, absl::string_view contents) {
  std::vector<std::string> lines = absl::StrSplit(contents, '\n');
  for (std::string& line : lines) {
    absl::StripAsciiWhitespace(&line);
  }
  return File(path, std::move(lines));
}

}  // namespace nsasm
