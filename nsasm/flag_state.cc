#include "nsasm/flag_state.h"

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"

namespace nsasm {

namespace {

std::string BitToNamePart(BitState bs) {
  switch (bs) {
    case B_on:
      return "8";
    case B_off:
      return "16";
    case B_original:
      return "*";
    case B_unknown:
    default:
      return "?";
  }
}

// If `*sv` begins with a valid name part, consume it and return its value.
// Otherwise return nullopt.
absl::optional<BitState> ConsumeNamePart(absl::string_view* sv) {
  if (sv->size() < 1) {
    return absl::nullopt;
  }
  const char c1 = (*sv)[0];
  if (c1 == '8') {
    sv->remove_prefix(1);
    return B_on;
  } else if (c1 == '*') {
    sv->remove_prefix(1);
    return B_original;
  } else if (c1 == '?') {
    sv->remove_prefix(1);
    return B_unknown;
  } else if (c1 == '1' && sv->size() > 1 && (*sv)[1] == '6') {
    sv->remove_prefix(2);
    return B_off;
  }
  return absl::nullopt;
}

bool ConsumeByte(absl::string_view* sv, char ch) {
  if (sv->size() > 0 && (*sv)[0] == ch) {
    sv->remove_prefix(1);
    return true;
  }
  return false;
}

}  // namespace

std::string FlagState::ToName() const {
  if (e_bit_ == B_off) {
    // native mode
    return absl::StrCat("m", BitToNamePart(m_bit_), "x", BitToNamePart(x_bit_));
  } else if (e_bit_ == B_on) {
    return "emu";
  } else {
    return "unk";
  }
}

std::string FlagState::ToString() const {
  const char* c_bit_str =
      (c_bit_ == B_on) ? ", c=1" : (c_bit_ == B_off) ? ", c=0" : "";
  return absl::StrCat(ToName(), c_bit_str);
}

absl::optional<FlagState> FlagState::FromName(absl::string_view name) {
  std::string lower_name_str = absl::AsciiStrToLower(name);
  absl::string_view lower_name = lower_name_str;

  if (lower_name == "unk") {
    return FlagState(B_unknown, B_unknown, B_unknown);
  } else if (lower_name == "emu") {
    return FlagState(B_on, B_on, B_on);
  }

  // The pattern must be "m" + name_part + "b" + name part
  if (!ConsumeByte(&lower_name, 'm')) return absl::nullopt;
  auto m_bit = ConsumeNamePart(&lower_name);
  if (!m_bit.has_value()) {
    return absl::nullopt;
  }
  if (!ConsumeByte(&lower_name, 'x')) return absl::nullopt;
  auto x_bit = ConsumeNamePart(&lower_name);

  if (!lower_name.empty()) {  // extra characters found
    return absl::nullopt;
  }

  return FlagState(B_off, *m_bit, *x_bit);
}

FlagState FlagState::Execute(const Instruction& i) const {
  FlagState new_state = *this;

  Mnemonic m = i.mnemonic;
  // Instructions that clear or set carry bit (used to prime the XCE
  // instruction, which swaps the carry bit and emulation bit.)
  //
  // BCC and BCS essentially set and clear the c bit for the next instruction,
  // respectively, because if the bit is in the opposite state, we will branch
  // instead.
  if (m == M_sec || m == M_bcc) {
    new_state.c_bit_ = B_on;
    return new_state;
  } else if (m == M_clc || m == M_bcs) {
    new_state.c_bit_ = B_off;
    return new_state;
  }

  // Instructions that clear or set status bits explicitly
  if (m == M_rep || m == M_sep) {
    BitState target = (m == M_rep) ? B_off : B_on;
    auto arg = i.arg1.Evaluate();
    if (!arg.ok()) {
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

FlagState FlagState::ExecuteBranch(const Instruction& i) const {
  FlagState new_state = Execute(i);
  Mnemonic m = i.mnemonic;
  if (m == M_bcc) {
    new_state.c_bit_ = B_off;
  } else if (m == M_bcs) {
    new_state.c_bit_ = B_on;
  }
  return new_state;
}

}  // namespace nsasm