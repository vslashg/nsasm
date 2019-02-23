#include "nsasm/flag_state.h"

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"

namespace nsasm {

namespace {

bool Known(BitState s) {
  return s == B_off || s == B_on;
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
  if (!Known(e_bit_)) {
    return "unk";
  } else if (e_bit_ == B_on) {
    return "emu";
  } else {
    absl::string_view m_str = !Known(m_bit_) ? "" : (m_bit_ == B_off) ? "m16" : "m8";
    absl::string_view x_str = !Known(x_bit_) ? "" : (x_bit_ == B_off) ? "x16" : "x8";
    if (m_str.empty() && x_str.empty()) {
      return "native";
    }
    return absl::StrCat(m_str, x_str);
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
  } else if (lower_name == "native") {
    return FlagState(B_off, B_original, B_original);
  } else if (lower_name.empty()) {
    return absl::nullopt;
  }

  BitState m_bit = B_original;
  BitState x_bit = B_original;
  if (ConsumeByte(&lower_name, 'm')) {
    auto read_m_bit = ConsumeNamePart(&lower_name);
    if (!read_m_bit.has_value()) {
      return absl::nullopt;
    }
    m_bit = *read_m_bit;
  }
  if (ConsumeByte(&lower_name, 'x')) {
    auto read_x_bit = ConsumeNamePart(&lower_name);
    if (!read_x_bit.has_value()) {
      return absl::nullopt;
    }
    x_bit = *read_x_bit;
  }
  if (!lower_name.empty()) {  // extra characters found
    return absl::nullopt;
  }

  return FlagState(B_off, m_bit, x_bit);
}

}  // namespace nsasm