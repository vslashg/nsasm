#include "nsasm/flag_state.h"

namespace nsasm {

FlagState FlagState::Execute(Mnemonic mnemonic, int arg1) const {
  FlagState new_state = *this;

  // Instructions that clear or set carry bit (used to prime the XCE
  // instruction, which swaps the carry bit and emulation bit.)
  if (mnemonic == M_sec) {
    new_state.c_bit_ = B_on;
    return new_state;
  } else if (mnemonic == M_clc) {
    new_state.c_bit_ = B_off;
    return new_state;
  }

  // Instructions that clear or set status bits explicitly
  if (mnemonic == M_rep || mnemonic == M_sep) {
    BitState target = (mnemonic == M_rep) ? B_off : B_on;
    if (arg1 & 0x01) {
      new_state.c_bit_ = target;
    }
    if (arg1 & 0x10) {
      new_state.x_bit_ = ConstrainedForEBit(target, e_bit_);
    }
    if (arg1 & 0x80) {
      new_state.m_bit_ = ConstrainedForEBit(target, e_bit_);
    }
    return new_state;
  }

  // Instructions that push or pull the status bits onto the stack.
  // This heuristic doesn't attempt to track the stack pointer; we
  // just assume a PLP instruction gets the last value pushed by PHP.
  if (mnemonic == M_php) {
    new_state.pushed_m_bit_ = m_bit_;
    new_state.pushed_x_bit_ = x_bit_;
    return new_state;
  } else if (mnemonic == M_plp) {
    new_state.m_bit_ = ConstrainedForEBit(pushed_m_bit_, e_bit_);
    new_state.x_bit_ = ConstrainedForEBit(pushed_m_bit_, e_bit_);
    new_state.pushed_m_bit_ = B_unknown;
    new_state.pushed_x_bit_ = B_unknown;
    return new_state;
  }

  // Instruction that swaps the c and e bits.  This can change the
  // m and x bits as a side effect.
  if (mnemonic == M_xce) {
    std::swap(new_state.c_bit_, new_state.e_bit_);
    new_state.m_bit_ = ConstrainedForEBit(m_bit_, new_state.e_bit_);
    new_state.x_bit_ = ConstrainedForEBit(x_bit_, new_state.e_bit_);
    return new_state;
  }

  // Instructions that use the c bit to indicate carry.  For the
  // purposes of this static analysis, treat these as setting the c
  // bit to an unknown state.
  if (mnemonic == M_adc || mnemonic == M_sbc || mnemonic == M_cmp ||
      mnemonic == M_cpx || mnemonic == M_cpy || mnemonic == M_asl ||
      mnemonic == M_lsr || mnemonic == M_rol || mnemonic == M_ror) {
    new_state.c_bit_ = B_unknown;
    return new_state;
  }

  // Other instructions don't effect the flag state.
  return new_state;
}

}  // namespace nsasm