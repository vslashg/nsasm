#ifndef NSASM_FLAG_STATE_H_
#define NSASM_FLAG_STATE_H_

#include <cstdint>

#include "absl/base/attributes.h"
#include "nsasm/mnemonic.h"

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
constexpr BitState operator|(BitState lhs, BitState rhs) {
  return (lhs == rhs) ? lhs : B_unknown;
}

// Returns the state of an `m` or `x` bit as constrained by an `e` bit.
//
// In emulation mode, the `m` and `x` registers are fixed at 1.  This function
// constrains a given `m` or `x` state given the current `e` state.
constexpr BitState ConstrainedForEBit(BitState input, BitState e) {
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
  // defaults to fully unknown state
  constexpr FlagState()
      : m_bit_(B_original),
        x_bit_(B_original),
        e_bit_(B_original),
        pushed_m_bit_(B_unknown),
        pushed_x_bit_(B_unknown),
        c_bit_(B_unknown) {}

  constexpr FlagState(BitState m_bit, BitState x_bit, BitState e_bit,
                      BitState pushed_m_bit, BitState pushed_x_bit,
                      BitState c_bit)
      : m_bit_(ConstrainedForEBit(m_bit, e_bit)),
        x_bit_(ConstrainedForEBit(x_bit, e_bit)),
        e_bit_(e_bit),
        pushed_m_bit_(pushed_m_bit),
        pushed_x_bit_(pushed_x_bit),
        c_bit_(c_bit) {}

  constexpr FlagState(const FlagState& rhs) = default;
  FlagState& operator=(const FlagState& rhs) = default;

  // The | operator merges two FlagStates into the superposition of their
  // states.  This is used to reflect all possible values for these bits
  // when an instruction can be reached over multiple code paths.
  friend constexpr FlagState operator|(FlagState lhs, FlagState rhs) {
    return FlagState(
        lhs.m_bit_ | rhs.m_bit_, lhs.x_bit_ | rhs.x_bit_,
        lhs.e_bit_ | rhs.e_bit_, lhs.pushed_m_bit_ | rhs.pushed_m_bit_,
        lhs.pushed_x_bit_ | rhs.pushed_x_bit_, lhs.c_bit_ | rhs.c_bit_);
  }

  FlagState& operator|=(FlagState rhs) {
    *this = *this | rhs;
    return *this;
  }

  // Returns the new state that results from executing the given instruction
  // from the current state.
  ABSL_MUST_USE_RESULT FlagState Execute(Mnemonic m, int arg1) const;

 private:
  BitState m_bit_;
  BitState x_bit_;
  BitState e_bit_;
  BitState pushed_m_bit_;
  BitState pushed_x_bit_;
  BitState c_bit_;
};

}  // namespace nsasm

#endif  // NSASM_FLAG_STATE_H_[