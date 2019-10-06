#ifndef NSASM_FILE_H_
#define NSASM_FILE_H_

#include <fstream>
#include <string>
#include <vector>

#include "nsasm/error.h"

namespace nsasm {

class File;

// Read the contents of the file at the given path into a File object.
nsasm::ErrorOr<File> OpenFile(const std::string& path);

// Construct a fake file object with the given contents.  Intended for testing.
nsasm::File MakeFakeFile(const std::string& path, absl::string_view contents);

// Abstraction for an .asm file.  Note that this just stores the full contents
// of the file in memory, which we get away with because of just how massively
// larger local RAM is than the target platform's.
class File {
 public:
  // Iterate over and access contents
  std::vector<std::string>::const_iterator begin() const {
    return lines_.begin();
  }
  std::vector<std::string>::const_iterator end() const { return lines_.end(); }
  const std::string& operator[](size_t i) const { return lines_[i]; }

  const std::string& path() const { return path_; }

 private:
  friend nsasm::ErrorOr<File> OpenFile(const std::string& path);
  friend nsasm::File MakeFakeFile(const std::string& path,
                                  absl::string_view contents);

  File(std::string path, std::vector<std::string> lines)
      : path_(path), lines_(std::move(lines)) {}
  std::string path_;
  std::vector<std::string> lines_;
};

}  // namespace nsasm

#endif  // NSASM_FILE_H_