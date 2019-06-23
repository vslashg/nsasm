#ifndef NSASM_FLAG_STATE_H_
#define NSASM_FLAG_STATE_H_

#include <cstdint>
#include <string>
#include <iostream>

#include "absl/base/attributes.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

// The flag state tracks whether the 65816 is in emulation mode or native mode,
// and if in native mode, the value of the `m` and `x` status bits.  It also
// tracks the status of the `c` (carry) bit, since the emulation bit can only
// bet set via the carry bit, and keeps limited track of the `m` and `x` status
// bits as they have been pushed on the stack.
//
// Bit states can be converted to names and back, for use in assembly directives
// and error messages.  These names do not fully express all possible bit combinations,
// but are intended to capture useful ones.
//
// The naming scheme is:
//   unknown (when the state is entirely unknown)
//   emu (`e` bit on)
//   native (native -- `e` bit off -- `m` and `x` bits unknown)
//   m8x8 (native mode, `m` and `x` bits on, 8 bits wide)
//   m8x16
//   m16x16
//   m8 (native, `m` bit on, `x` bit in unknown state)
//   x16 (native, `x` bit off, `m` bit in unknown state)

namespace nsasm {

// Possible states of a status bit, for the purposes of static analysis.
enum BitState : uint8_t {
  // Bit is known to be zero
  B_off,
  // Bit is known to be one
  B_on,
  // Bit is known to be set to its initial value (on subroutine entrance);
  // the actual value is unknown.
  B_original,
  // Bit value is unknown.
  B_unknown,
};

// Returns the merged value of two states for the same bit.  Needed when an
// instruction can be entered via multiple code paths.
inline BitState operator|(BitState lhs, BitState rhs) {
  return (lhs == rhs) ? lhs : B_unknown;
}

// Returns the state of an `m` or `x` bit as constrained by an `e` bit.
//
// In emulation mode, the `m` and `x` registers are fixed at 1.  This function
// constrains a given `m` or `x` state given the current `e` state.
inline BitState ConstrainedForEBit(BitState input, BitState e) {
  // In emulation mode, `m` and `x` are always on.
  if (e == B_on) {
    return B_on;
  }
  // In native mode, `m` and `x` can be any value.
  if (e == B_off) {
    return input;
  }
  // `m` and `x` can be set to 1 in both native and emulation mode, so B_on
  // is always a valid state for these bits.
  if (input == B_on) {
    return B_on;
  }
  // If no status bits have been changed in this subroutine, they remain
  // at their original values.
  if (input == B_original && e == B_original) {
    return B_original;
  }
  // When we don't know the state of the `e` bit, and we're not sure that
  // `m` or `x` are at 1, we don't know enough to predict the bit values.
  return B_unknown;
}

// Data type to track the current possible runtime states of the status bits (n,
// x, and e).  This is a limited form of static analysis, used to determine how
// to encode immediate values in instructions like `ADC #$42`, where instruction
// sizes depend on the state of the flags.
class FlagState {
 public:
  // Defaults to assuming 65816 native mode, but with otherwise unknown state.
  explicit FlagState(BitState e_bit = B_off, BitState m_bit = B_original,
                     BitState x_bit = B_original,
                     BitState pushed_m_bit = B_unknown,
                     BitState pushed_x_bit = B_unknown,
                     BitState c_bit = B_unknown)
      : e_bit_(e_bit),
        m_bit_(ConstrainedForEBit(m_bit, e_bit)),
        x_bit_(ConstrainedForEBit(x_bit, e_bit)),
        pushed_m_bit_(pushed_m_bit),
        pushed_x_bit_(pushed_x_bit),
        c_bit_(c_bit) {}

  FlagState(const FlagState& rhs) = default;
  FlagState& operator=(const FlagState& rhs) = default;

  // Returns a FlagState reflecting the given name, or `nullopt` if the name is
  // not valid.
  static absl::optional<FlagState> FromName(absl::string_view name);

  BitState EBit() const { return e_bit_; }
  BitState MBit() const { return m_bit_; }
  BitState XBit() const { return x_bit_; }
  BitState CBit() const { return c_bit_; }

  void SetMBit(BitState state) { m_bit_ = ConstrainedForEBit(state, e_bit_); }
  void SetXBit(BitState state) { x_bit_ = ConstrainedForEBit(state, e_bit_); }
  void SetCBit(BitState state) { c_bit_ = state; }

  void PushFlags() {
    pushed_m_bit_ = m_bit_;
    pushed_x_bit_ = x_bit_;
  }

  void PullFlags() {
    m_bit_ = ConstrainedForEBit(pushed_m_bit_, e_bit_);
    x_bit_ = ConstrainedForEBit(pushed_m_bit_, e_bit_);
    pushed_m_bit_ = B_unknown;
    pushed_x_bit_ = B_unknown;
  }

  void ExchangeCE() {
    std::swap(c_bit_, e_bit_);
    m_bit_ = ConstrainedForEBit(m_bit_, e_bit_);
    x_bit_ = ConstrainedForEBit(x_bit_, e_bit_);
  }

  // Returns the name of this flag state.
  std::string ToName() const;

  // Returns a human-readable representation of this flag state.
  std::string ToString() const;

  // The | operator merges two FlagStates into the superposition of their
  // states.  This is used to reflect all possible values for these bits
  // when an instruction can be reached over multiple code paths.
  friend FlagState operator|(const FlagState& lhs, const FlagState& rhs) {
    return FlagState(
        lhs.e_bit_ | rhs.e_bit_, lhs.m_bit_ | rhs.m_bit_,
        lhs.x_bit_ | rhs.x_bit_, lhs.pushed_m_bit_ | rhs.pushed_m_bit_,
        lhs.pushed_x_bit_ | rhs.pushed_x_bit_, lhs.c_bit_ | rhs.c_bit_);
  }

  FlagState& operator|=(const FlagState& rhs) {
    *this = *this | rhs;
    return *this;
  }

  bool operator==(const FlagState& rhs) const {
    return std::tie(e_bit_, m_bit_, x_bit_, pushed_m_bit_, pushed_x_bit_,
                    c_bit_) ==  //
           std::tie(rhs.e_bit_, rhs.m_bit_, rhs.x_bit_, rhs.pushed_m_bit_,
                    rhs.pushed_x_bit_, rhs.c_bit_);
  }

  bool operator!=(const FlagState& rhs) const { return !(*this == rhs); }

 private:
  BitState e_bit_;
  BitState m_bit_;
  BitState x_bit_;
  BitState pushed_m_bit_;
  BitState pushed_x_bit_;
  BitState c_bit_;
};

// googletest pretty printer (streams are hot garbage)
inline void PrintTo(BitState v, std::ostream* out) {
  if (v == B_on) {
    *out << "B_on";
  } else if (v == B_off) {
    *out << "B_off";
  } else if (v == B_unknown) {
    *out << "B_unknown";
  } else if (v == B_original) {
    *out << "B_orignal";
  } else {
    *out << static_cast<int>(v);
  }
}


}  // namespace nsasm

#endif  // NSASM_FLAG_STATE_H_[