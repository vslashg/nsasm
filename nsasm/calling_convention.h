#ifndef NSASM_CALLING_CONVENTION_H_
#define NSASM_CALLING_CONVENTION_H_

#include "absl/types/variant.h"
#include "nsasm/flag_state.h"

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
  ReturnConvention(const FlagState& state) : state_(state) {}

  // Constructs a "noreturn" convention.
  ReturnConvention(NoReturn) : state_(NoReturn()) {}

  // This is a cheaply copiable type.
  ReturnConvention(const ReturnConvention&) = default;
  ReturnConvention& operator=(const ReturnConvention&) = default;

  absl::optional<FlagState> YieldState() const {
    auto return_mode = absl::get_if<FlagState>(&state_);
    if (!return_mode) {
      return absl::nullopt;
    }
    return *return_mode;
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
  absl::variant<absl::monostate, FlagState, NoReturn> state_;
};

struct CallingConvention {
  FlagState incoming_state;
  ReturnConvention return_state;
};

}  // namespace nsasm

#endif  // NSASM_CALLING_CONVENTION_H_
