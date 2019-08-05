#ifndef NSASM_CALLING_CONVENTION_H_
#define NSASM_CALLING_CONVENTION_H_

#include "absl/types/variant.h"
#include "nsasm/execution_state.h"

namespace nsasm {

// Tag type representing a return
struct NoReturn {};

// Value type representing how a subroutine returns.  This can take three
// states:
//
// 1) default: status bits are preserved by the subroutine.
// 2) yields: status bits are changed to a specified state before return.
// 3) noreturn: execution never returns from the JSR/JSL.
class ReturnConvention {
 public:
  // Default constructed creates the "default" state.
  ReturnConvention() : state_() {}

  // Constructs a "yields" convention.
  ReturnConvention(const StatusFlags& flags) : state_(flags) {}

  // Constructs a "noreturn" convention.
  ReturnConvention(NoReturn) : state_(NoReturn()) {}

  // This is a cheaply copiable type.
  ReturnConvention(const ReturnConvention&) = default;
  ReturnConvention& operator=(const ReturnConvention&) = default;

  absl::optional<StatusFlags> YieldFlags() const {
    auto return_mode = absl::get_if<StatusFlags>(&state_);
    if (!return_mode) {
      return absl::nullopt;
    }
    return *return_mode;
  }

  void ApplyTo(nsasm::ExecutionState* state) const {
    auto return_mode = absl::get_if<StatusFlags>(&state_);
    if (return_mode) {
      state->Flags() = *return_mode;
    }
  }

  // Returns true iff this is the default convention.
  bool IsDefault() const {
    return absl::holds_alternative<absl::monostate>(state_);
  }
  // Returns true iff this call never returns.
  bool IsExitCall() const {
    return absl::holds_alternative<NoReturn>(state_);
  }

  // Convert the return convention to a suffix string, suitable for appending
  // to a JSR or JSL instruction or an .entry or .remote directive.
  //
  // In the default state, this returns an empty string.
  std::string ToSuffixString() const;
 private:
  absl::variant<absl::monostate, StatusFlags, NoReturn> state_;
};

struct CallingConvention {
  StatusFlags incoming_state;
  ReturnConvention return_state;
};

}  // namespace nsasm

#endif  // NSASM_CALLING_CONVENTION_H_
