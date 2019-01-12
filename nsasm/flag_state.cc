#include "nsasm/flag_state.h"

namespace nsasm {

FlagState FlagState::Execute(Instruction i) const {
  FlagState new_state = *this;

  Mnemonic m = i.mnemonic;
  // Instructions that clear or set carry bit (used to prime the XCE
  // instruction, which swaps the carry bit and emulation bit.)
  if (m == M_sec) {
    new_state.c_bit_ = B_on;
    return new_state;
  } else if (m == M_clc) {
    new_state.c_bit_ = B_off;
    return new_state;
  }

  // Instructions that clear or set status bits explicitly
  if (m == M_rep || m == M_sep) {
    BitState target = (m == M_rep) ? B_off : B_on;
    auto arg = i.arg1.ToValue();
    if (!arg.has_value()) {
      // If REP or SEP are invoked with an unknown argument (a constant pulled
      // from another module, say), we will have to account for the ambiguity.
      //
      // Each bit will either be set to `target` or else left alone.  If the
      // current value of a bit is equal to `target`, it's unchanged; otherwise
      // it becomes ambiguous.
      if (c_bit_ != target) {
        new_state.c_bit_ = B_unknown;
      }
      if (x_bit_ != target) {
        new_state.x_bit_ = ConstrainedForEBit(B_unknown, e_bit_);
      }
      if (m_bit_ != target) {
        new_state.m_bit_ = ConstrainedForEBit(B_unknown, e_bit_);
      }
      return new_state;
    }

    // If the argument is known, we can set the effected bits.
    if (*arg & 0x01) {
      new_state.c_bit_ = target;
    }
    if (*arg & 0x10) {
      new_state.x_bit_ = ConstrainedForEBit(target, e_bit_);
    }
    if (*arg & 0x20) {
      new_state.m_bit_ = ConstrainedForEBit(target, e_bit_);
    }
    return new_state;
  }

  // Instructions that push or pull the status bits onto the stack.
  // This heuristic doesn't attempt to track the stack pointer; we
  // just assume a PLP instruction gets the last value pushed by PHP.
  if (m == M_php) {
    new_state.pushed_m_bit_ = m_bit_;
    new_state.pushed_x_bit_ = x_bit_;
    return new_state;
  } else if (m == M_plp) {
    new_state.m_bit_ = ConstrainedForEBit(pushed_m_bit_, e_bit_);
    new_state.x_bit_ = ConstrainedForEBit(pushed_m_bit_, e_bit_);
    new_state.pushed_m_bit_ = B_unknown;
    new_state.pushed_x_bit_ = B_unknown;
    return new_state;
  }

  // Instruction that swaps the c and e bits.  This can change the
  // m and x bits as a side effect.
  if (m == M_xce) {
    std::swap(new_state.c_bit_, new_state.e_bit_);
    new_state.m_bit_ = ConstrainedForEBit(m_bit_, new_state.e_bit_);
    new_state.x_bit_ = ConstrainedForEBit(x_bit_, new_state.e_bit_);
    return new_state;
  }

  // Instructions that use the c bit to indicate carry.  For the
  // purposes of this static analysis, treat these as setting the c
  // bit to an unknown state.
  if (m == M_adc || m == M_sbc || m == M_cmp || m == M_cpx || m == M_cpy ||
      m == M_asl || m == M_lsr || m == M_rol || m == M_ror) {
    new_state.c_bit_ = B_unknown;
    return new_state;
  }

  // Other instructions don't effect the flag state.
  return new_state;
}

}  // namespace nsasm