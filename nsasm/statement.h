#ifndef NSASM_STATEMENT_H
#define NSASM_STATEMENT_H

#include "nsasm/directive.h"
#include "nsasm/instruction.h"

namespace nsasm {

// A line in an .asm file, either a directive or a line of assembly.
class Statement {
 public:
  explicit Statement(nsasm::Instruction i) : data_(std::move(i)) {}
  explicit Statement(nsasm::Directive i) : data_(std::move(i)) {}

  Statement(const Statement&) = default;
  Statement(Statement&&) = default;

  Statement& operator=(const Statement&) = delete;
  Statement& operator=(Statement&&) = default;

  const nsasm::Instruction* Instruction() const {
    return absl::get_if<nsasm::Instruction>(&data_);
  }
  const nsasm::Directive* Directive() const {
    return absl::get_if<nsasm::Directive>(&data_);
  }

  std::string ToString() const;

 private:
  absl::variant<nsasm::Instruction, nsasm::Directive> data_;
};

}  // namespace nsasm

#endif  // NSASM_STATEMENT_H
