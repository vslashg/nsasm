#ifndef NSASM_STATEMENT_H
#define NSASM_STATEMENT_H

#include "nsasm/address.h"
#include "nsasm/directive.h"
#include "nsasm/instruction.h"
#include "nsasm/location.h"

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
  nsasm::Instruction* Instruction() {
    return absl::get_if<nsasm::Instruction>(&data_);
  }
  const nsasm::Directive* Directive() const {
    return absl::get_if<nsasm::Directive>(&data_);
  }
  nsasm::Directive* Directive() {
    return absl::get_if<nsasm::Directive>(&data_);
  }

  bool operator==(Mnemonic m) const {
    auto* i = Instruction();
    return i != nullptr && i->mnemonic == m;
  }
  bool operator==(DirectiveName dn) const {
    auto* d = Directive();
    return d != nullptr && d->name == dn;
  }

  nsasm::Location Location() const;

  ErrorOr<void> Execute(ExecutionState* es) const;

  ErrorOr<void> ExecuteBranch(ExecutionState* es) const;

  int SerializedSize() const;

  ErrorOr<void> Assemble(nsasm::Address address, const LookupContext& context,
                         OutputSink* sink) const;

  bool IsLocalBranch() const {
    auto* ins = absl::get_if<nsasm::Instruction>(&data_);
    return ins && ins->IsLocalBranch();
  }

  bool IsExitInstruction() const;

  std::string ToString() const;

 private:
  absl::variant<nsasm::Instruction, nsasm::Directive> data_;
};

inline bool operator==(Mnemonic m, const Statement& s) { return s == m; }
inline bool operator==(DirectiveName dn, const Statement& s) { return s == dn; }
inline bool operator!=(const Statement& s, Mnemonic m) { return !(s == m); }
inline bool operator!=(const Statement& s, DirectiveName dn) {
  return !(s == dn);
}
inline bool operator!=(Mnemonic m, const Statement& s) { return !(s == m); }
inline bool operator!=(DirectiveName dn, const Statement& s) {
  return !(s == dn);
}

}  // namespace nsasm

#endif  // NSASM_STATEMENT_H
