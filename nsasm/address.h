#ifndef NSASM_ADDRESS_H
#define NSASM_ADDRESS_H

#include <cstdint>
#include <string>

#include "absl/base/attributes.h"
#include "absl/strings/str_format.h"
#include "nsasm/error.h"
#include "nsasm/location.h"
#include "nsasm/numeric_type.h"

namespace nsasm {

// An absolute address in the 65816 address space.
class Address {
 public:
  constexpr Address() : value_(0) {}
  explicit Address(uint32_t value) : value_(value) {
    // Allow 0x1000000 as an address, as a one-past-the-end index into memory
    // space in ranges.h.
    assert(value <= 0x1000000u);
  }
  explicit Address(uint8_t bank, uint16_t bank_address)
      : value_((bank << 16) | bank_address) {}

  constexpr int Bank() const { return value_ >> 16; }
  constexpr uint16_t BankAddress() const { return value_ & 0xffffu; }

  operator Location() const { return nsasm::Location::FromAddress(value_); }

  std::string ToString() const { return absl::StrFormat("$%06x", value_); }

  // Return this address, adjusted by the given offset.
  // Wraps within the current bank (the high 8 bits remain unchanged.)
  ABSL_MUST_USE_RESULT Address AddWrapped(int offset) const {
    const uint32_t high = value_ & 0xff0000u;
    const uint32_t low = (value_ + offset) & 0xffffu;
    return Address(high | low);
  }

  ABSL_MUST_USE_RESULT Address AddUnwrapped(int offset) const {
    return Address(value_ + offset);
  }

  // Subtract rhs from this, but only if the two addresses are in the same bank.
  //
  // This method understands wrapping, and will return an offset in the range
  // -32768 to 32767.  (For example, 0x050000 - 0x05ffff will return 1, not
  // -65535.)
  ErrorOr<int> SubtractWrapped(Address rhs) const {
    if (Bank() != rhs.Bank()) {
      return Error("Subtracting ");
    }
    int offset =
        (unsigned(BankAddress()) - unsigned(rhs.BankAddress())) & 0xffffu;
    return (offset > 0x7fff) ? offset - 0x10000 : offset;
  }

  constexpr bool operator==(Address rhs) const { return value_ == rhs.value_; }
  constexpr bool operator!=(Address rhs) const { return value_ != rhs.value_; }
  constexpr bool operator<(Address rhs) const { return value_ < rhs.value_; }
  constexpr bool operator<=(Address rhs) const { return value_ <= rhs.value_; }
  constexpr bool operator>(Address rhs) const { return value_ > rhs.value_; }
  constexpr bool operator>=(Address rhs) const { return value_ >= rhs.value_; }

  template <typename H>
  friend H AbslHashValue(H h, Address a) {
    return H::combine(std::move(h), a.value_);
  }

 private:
  uint32_t value_;
};

inline std::ostream& operator<<(std::ostream& os, nsasm::Address ad) {
  return os << ad.ToString();
}

class LabelValue {
 public:
  constexpr LabelValue() : value_(0) {}
  constexpr LabelValue(Address a)
      : value_((a.Bank() << 16) | a.BankAddress()) {}
  static LabelValue FromInt(int i) {
    LabelValue v;
    v.value_ = i;
    return v;
  }

  int ToNumber(NumericType type) const { return CastTo(type, value_); }

  int ToInt() const { return value_; }

  Address ToAddress() const { return Address(value_); }

  constexpr bool operator==(LabelValue rhs) const {
    return value_ == rhs.value_;
  }
  constexpr bool operator!=(LabelValue rhs) const {
    return value_ != rhs.value_;
  }
  constexpr bool operator<(LabelValue rhs) const { return value_ < rhs.value_; }
  constexpr bool operator<=(LabelValue rhs) const {
    return value_ <= rhs.value_;
  }
  constexpr bool operator>(LabelValue rhs) const { return value_ > rhs.value_; }
  constexpr bool operator>=(LabelValue rhs) const {
    return value_ >= rhs.value_;
  }

  template <typename H>
  friend H AbslHashValue(H h, LabelValue a) {
    return H::combine(std::move(h), a.value_);
  }

 private:
  int value_;
};

}  // namespace nsasm

#endif  // NSASM_ADDRESS_H
