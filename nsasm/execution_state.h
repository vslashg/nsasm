#ifndef NSASM_EXECUTION_STATE_H_
#define NSASM_EXECUTION_STATE_H_

#include <cstdint>

#include "absl/container/inlined_vector.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace nsasm {

// Possible static analysis states of a status register bit.
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

// Returns the state of an `m` or `x` bit as constrained by an `e` bit.
//
// In emulation mode, the `m` and `x` bits are fixed at 1.  (Technically, the
// `x` bit doesn't exist, but it functions as on.)  This function constrains a
// given `m` or `x` state given the current `e` state.
inline BitState ConstrainedForEBit(BitState input, BitState e) {
  // In emulation mode, `m` and `x` are always on.
  if (e == B_on) {
    return B_on;
  }
  // In native mode, `m` and `x` can be any value, so there's nothing to
  // constrain.
  if (e == B_off) {
    return input;
  }
  // `m` and `x` can be set to 1 in both native and emulation mode, so B_on
  // is always a valid state for these bits.
  if (input == B_on) {
    return B_on;
  }
  // If no status bits have been changed yet, they remain at their original
  // values.
  if (input == B_original && e == B_original) {
    return B_original;
  }
  // When we don't know the state of the `e` bit, and we're not sure that
  // `m` or `x` are at 1, we don't know enough to predict the bit values.
  return B_unknown;
}

// Returns the merged value of two states for the same bit.  Needed when an
// instruction can be entered via multiple code paths.
inline BitState operator|(BitState lhs, BitState rhs) {
  return (lhs == rhs) ? lhs : B_unknown;
}

class StatusFlags {
 public:
  // Defaults to fully unknown state
  explicit StatusFlags(BitState e_bit = B_unknown, BitState m_bit = B_unknown,
                       BitState x_bit = B_unknown, BitState c_bit = B_unknown)
      : e_bit_(e_bit),
        m_bit_(ConstrainedForEBit(m_bit, e_bit)),
        x_bit_(ConstrainedForEBit(x_bit, e_bit)),
        c_bit_(c_bit) {}

  StatusFlags(const StatusFlags& rhs) = default;
  StatusFlags& operator=(const StatusFlags& rhs) = default;

  // Returns a StatusFlags reflecting the given name, or `nullopt` if the name
  // is not valid.
  static absl::optional<StatusFlags> FromName(absl::string_view name);

  BitState EBit() const { return BitState(e_bit_); }
  BitState MBit() const { return BitState(m_bit_); }
  BitState XBit() const { return BitState(x_bit_); }
  BitState CBit() const { return BitState(c_bit_); }

  void SetMBit(BitState state) { m_bit_ = ConstrainedForEBit(state, EBit()); }
  void SetXBit(BitState state) { x_bit_ = ConstrainedForEBit(state, EBit()); }
  void SetCBit(BitState state) { c_bit_ = state; }

  // Modify this flag state to represent an "incoming" state to a subroutine.
  // All "unknown" bits become "original".
  void SetIncoming() {
    if (e_bit_ == B_unknown) e_bit_ = B_original;
    if (m_bit_ == B_unknown) m_bit_ = B_original;
    if (x_bit_ == B_unknown) x_bit_ = B_original;
    if (c_bit_ == B_unknown) c_bit_ = B_original;
  }

  void ExchangeCE() {
    // "I'd use std::swap here, but c_bit_ and e_bit_ are bitfield members,
    // and thus can't be passed to functions by reference."
    // "Sir, this is an Arby's."
    uint8_t old_c_bit = c_bit_;
    c_bit_ = e_bit_;
    e_bit_ = old_c_bit;
    m_bit_ = ConstrainedForEBit(MBit(), EBit());
    x_bit_ = ConstrainedForEBit(XBit(), EBit());
  }

  // Returns the name of this flag state.
  std::string ToName() const;

  // Returns a human-readable representation of this flag state.
  std::string ToString() const;

  // The | operator merges two FlagStates into the superposition of their
  // states.  This is used to reflect all possible values for these bits
  // when an instruction can be reached over multiple code paths.
  friend StatusFlags operator|(const StatusFlags& lhs, const StatusFlags& rhs) {
    return StatusFlags(lhs.EBit() | rhs.EBit(), lhs.MBit() | rhs.MBit(),
                       lhs.XBit() | rhs.XBit(), lhs.CBit() | rhs.CBit());
  }

  StatusFlags& operator|=(const StatusFlags& rhs);

  bool operator==(const StatusFlags& rhs) const {
    return e_bit_ == rhs.e_bit_ && m_bit_ == rhs.m_bit_ &&
           x_bit_ == rhs.x_bit_ && c_bit_ == rhs.c_bit_;
  }

  bool operator!=(const StatusFlags& rhs) const { return !(*this == rhs); }

 private:
  uint8_t e_bit_ : 2;
  uint8_t m_bit_ : 2;
  uint8_t x_bit_ : 2;
  uint8_t c_bit_ : 2;
};

class RegisterValue {
 public:
  enum Type : int16_t {
    T_unknown,
    T_original,
    T_value,
  };

  RegisterValue(Type type = T_unknown) : type_(type), value_(0) {}
  RegisterValue(uint16_t value) : type_(T_value), value_(value) {}

  RegisterValue(const RegisterValue&) = default;
  RegisterValue& operator=(const RegisterValue&) = default;

  Type type() const { return type_; }
  bool HasValue() const { return type_ == T_value; }
  uint16_t operator*() const { return value_; }

  void Add(int offset, int mask) {
    if (type_ == T_value) {
      value_ = (value_ + offset) & mask;
    } else {
      type_ = T_unknown;
      value_ = 0;
    }
  }

  bool operator==(const RegisterValue& rhs) const {
    return type_ == rhs.type_ && value_ == rhs.value_;
  }

  bool operator!=(const RegisterValue& rhs) const { return !(*this == rhs); }

  RegisterValue& operator|=(const RegisterValue& rhs) {
    if (type_ != rhs.type_ || value_ != rhs.value_) {
      type_ = T_unknown;
      value_ = 0;
    }
    return *this;
  }

 private:
  Type type_;
  uint16_t value_;
};

inline StatusFlags& StatusFlags::operator|=(const StatusFlags& rhs) {
  *this = *this | rhs;
  return *this;
}

// Things that can be in a stack position:
//
// * Unknown
// * Literal byte
// * Flags register state
// * {A,X,Y}_{hi,lo} original
// * DBR original
//
// Registers to track:
// FlagState (complex)
//
// Value or unknown: A, X, Y, DBR

struct StackValue {
 public:
  enum Type : uint8_t {
    T_unknown,
    T_byte,
    T_flags,
    T_a_hi,
    T_a_lo,
    T_a_varsize,
    T_x_hi,
    T_x_lo,
    T_x_varsize,
    T_y_hi,
    T_y_lo,
    T_y_varsize,
    T_dbr,
  };

  StackValue() : type_(T_unknown), value_(0) {}
  StackValue(uint8_t value) : type_(T_byte), value_(value) {}
  StackValue(StatusFlags flags) : type_(T_flags), flags_(flags) {}

  // Stack value representing the given byte of a given register.  This does
  // the right thing for the given RegisterValue.  If the register is in its
  // unknown but original state, we set the matching mode for this entry.
  // If it's unknown, this stack entry is unknown too.  If the register's value
  // is known, we set this to the correct constant value.
  StackValue(Type type, RegisterValue reg) : value_(0) {
    if (type == T_a_hi || type == T_x_hi || type == T_y_hi) {
      // Pushing the high byte of a register
      if (reg.type() == RegisterValue::T_original) {
        type_ = type;
      } else if (reg.type() == RegisterValue::T_unknown) {
        type_ = T_unknown;
      } else {
        type_ = T_byte;
        value_ = (*reg >> 8) & 0xff;
      }
    } else if (type == T_a_lo || type == T_x_lo || type == T_y_lo ||
               type == T_dbr) {
      // Pushing the low byte of a register
      if (reg.type() == RegisterValue::T_original) {
        type_ = type;
      } else if (reg.type() == RegisterValue::T_unknown) {
        type_ = T_unknown;
      } else {
        type_ = T_byte;
        value_ = *reg & 0xff;
      }
    } else if (type == T_a_varsize || type == T_x_varsize ||
               type == T_y_varsize) {
      type_ = type;
      if (reg.type() == RegisterValue::T_original) {
        // value == 1 for varsize represents original value is held
        value_ = 1;
      }
    } else {
      // Shouldn't get here
      assert(false);
      type_ = T_unknown;
    }
  }

  // Merges two potential stack values into one combined entry
  StackValue& operator|=(const StackValue& rhs) {
    if (type_ != rhs.type_) {
      type_ = T_unknown;
      value_ = 0;
      return *this;
    }
    // Same type; merge the underlying value for stateful types
    if (type_ == T_byte) {
      if (value_ != rhs.value_) {
        type_ = T_unknown;
        value_ = 0;
      }
    } else if (type_ == T_flags) {
      flags_ |= rhs.flags_;
    } else if (type_ == T_a_varsize || type_ == T_x_varsize ||
               type_ == T_y_varsize) {
      if (value_ != rhs.value_) {
        value_ = 0;
      }
    }
    return *this;
  }

  bool CanMergeWith(const StackValue& rhs) const {
    return type_ == rhs.type_ || (!IsVarSize() && !rhs.IsVarSize());
  }

  Type type() const { return type_; }
  uint8_t value() const {
    assert(type_ == T_byte);
    return value_;
  }
  StatusFlags flags() const {
    assert(type_ == T_flags);
    return flags_;
  }
  bool IsVarSize() const {
    return type_ == T_a_varsize || type_ == T_x_varsize || type_ == T_y_varsize;
  }

  bool operator==(const StackValue& rhs) const {
    if (type_ != rhs.type_) {
      return false;
    } else if (type_ == T_flags) {
      return flags_ == rhs.flags_;
    }
    return value_ == rhs.value_;
  }

 private:
  Type type_;
  union {
    uint8_t value_;
    StatusFlags flags_;
  };
};

// Static analysis representation of the stack
class Stack {
 public:
  // Construct an empty stack (the state at the start of a subroutine).
  Stack() : abandoned_(false), stack_() {}

  // Abandon stack analysis.  (Used when static analysis leads to an uncertain
  // or inconsistent stack state.)
  void Abandon() {
    abandoned_ = true;
    stack_.clear();
  }

  void PushByte(uint8_t value) {
    if (!abandoned_) {
      stack_.push_back(StackValue(value));
    }
  }

  void PushByte(absl::optional<uint8_t> value) {
    if (value.has_value()) {
      PushByte(*value);
    } else {
      PushUnknownByte();
    }
  }

  void PushUnknownByte() {
    if (!abandoned_) {
      stack_.push_back(StackValue());
    }
  }

  void PushWord(uint16_t value) {
    // Stack grows downward in memory, so pushing the high byte first results
    // in correct endianness. (Source: resident endianness expert Pixel.)
    PushByte((value >> 8) & 0xff);
    PushByte(value & 0xff);
  }

  void PushWord(absl::optional<uint16_t> value) {
    if (value.has_value()) {
      PushWord(*value);
    } else {
      PushUnknownWord();
    }
  }

  void PushUnknownWord() {
    if (!abandoned_) {
      stack_.push_back(StackValue());
      stack_.push_back(StackValue());
    }
  }

  StackValue PullByte() {
    if (abandoned_ || stack_.empty()) {
      Abandon();
      return StackValue();
    }
    StackValue result = stack_.back();
    if (result.IsVarSize()) {
      Abandon();
      return StackValue();
    }
    stack_.pop_back();
    return result;
  }

  // Pull a variable-sized object from the stack.  If that's not what's on top
  // of the stack, this fails.
  StackValue PullVarsize() {
    if (abandoned_) {
      return StackValue();
    }
    StackValue result = stack_.back();
    if (!result.IsVarSize()) {
      Abandon();
      return StackValue();
    }
    stack_.pop_back();
    return result;
  }

 private:
  void PushReg(RegisterValue reg, BitState bit, StackValue::Type lo_byte_type,
               StackValue::Type hi_byte_type, StackValue::Type var_byte_type) {
    if (!abandoned_) {
      if (bit == B_original) {
        stack_.emplace_back(var_byte_type, reg);
      } else if (bit == B_unknown) {
        Abandon();
      } else if (bit == B_on) {
        // 8-bit register
        stack_.emplace_back(lo_byte_type, reg);
      } else {  // bit == B_off
        stack_.emplace_back(hi_byte_type, reg);
        stack_.emplace_back(lo_byte_type, reg);
      }
    }
  }

  RegisterValue PullReg(BitState bit, StackValue::Type lo_byte_type,
                        StackValue::Type hi_byte_type,
                        StackValue::Type var_byte_type) {
    if (abandoned_ || bit == B_unknown) {
      Abandon();
      return RegisterValue();
    } else if (bit == B_original) {
      StackValue result = PullVarsize();
      if (result.type() == var_byte_type) {
        if (result.value() == 1) {
          // holds original value
          return RegisterValue(RegisterValue::T_original);
        } else {
          // holds dynamic sized data of unknown value; don't abandon
          return RegisterValue();
        }
      } else {
        Abandon();
        return RegisterValue();
      }
    } else if (bit == B_on) {
      // 8-bit register
      StackValue byte = PullByte();
      if (byte.type() == lo_byte_type) {
        return RegisterValue(RegisterValue::T_original);
      } else if (byte.type() == StackValue::T_byte) {
        return RegisterValue(byte.value());
      } else {
        return RegisterValue();
      }
    } else {  // bit == B_off
      // 16-bit register
      StackValue lo_byte = PullByte();
      StackValue hi_byte = PullByte();
      if (lo_byte.type() == lo_byte_type && hi_byte.type() == hi_byte_type) {
        return RegisterValue(RegisterValue::T_original);
      } else if (lo_byte.type() == StackValue::T_byte &&
                 hi_byte.type() == StackValue::T_byte) {
        return RegisterValue((lo_byte.value() << 8) + hi_byte.value());
      } else {
        return RegisterValue();
      }
    }
  }

 public:
  // Push the contents of the provided A register
  void PushA(RegisterValue a_reg, StatusFlags flags) {
    PushReg(a_reg, flags.MBit(), StackValue::T_a_lo, StackValue::T_a_hi,
            StackValue::T_a_varsize);
  }

  void PushX(RegisterValue x_reg, StatusFlags flags) {
    PushReg(x_reg, flags.XBit(), StackValue::T_x_lo, StackValue::T_x_hi,
            StackValue::T_x_varsize);
  }

  void PushY(RegisterValue y_reg, StatusFlags flags) {
    PushReg(y_reg, flags.XBit(), StackValue::T_y_lo, StackValue::T_y_hi,
            StackValue::T_y_varsize);
  }

  void PushDBR(RegisterValue dbr) {
    if (!abandoned_) {
      stack_.emplace_back(StackValue::T_dbr, dbr);
    }
  }

  void PushFlags(StatusFlags flags) {
    if (!abandoned_) {
      stack_.emplace_back(flags);
    }
  }

  RegisterValue PullA(StatusFlags flags) {
    return PullReg(flags.MBit(), StackValue::T_a_lo, StackValue::T_a_hi,
                   StackValue::T_a_varsize);
  }

  RegisterValue PullX(StatusFlags flags) {
    return PullReg(flags.XBit(), StackValue::T_x_lo, StackValue::T_x_hi,
                   StackValue::T_x_varsize);
  }

  RegisterValue PullY(StatusFlags flags) {
    return PullReg(flags.XBit(), StackValue::T_y_lo, StackValue::T_y_hi,
                   StackValue::T_y_varsize);
  }

  Stack& operator|=(const Stack& rhs) {
    if (abandoned_ || rhs.abandoned_ || stack_.size() != rhs.stack_.size()) {
      Abandon();
    } else {
      for (size_t i = 0; i < stack_.size(); ++i) {
        if (!stack_[i].CanMergeWith(rhs.stack_[i])) {
          Abandon();
          return *this;
        }
        stack_[i] |= rhs.stack_[i];
      }
    }
    return *this;
  }

  bool operator==(const Stack& rhs) const {
    return abandoned_ == rhs.abandoned_ && stack_ == rhs.stack_;
  }

 private:
  bool abandoned_;
  absl::InlinedVector<StackValue, 16> stack_;
};

// Representation of the execution state on a line.  A little on the big side,
// but this is still a value type.
class ExecutionState {
 public:
  // default execution state is all registers unknown, and stack clear.
  ExecutionState() {}

  // Initial execution state for a given flag state.
  ExecutionState(const StatusFlags& flags) : flags_(flags) {
    flags_.SetIncoming();
  }

  ExecutionState(const ExecutionState&) = default;
  ExecutionState(ExecutionState&&) = default;
  ExecutionState& operator=(const ExecutionState&) = default;
  ExecutionState& operator=(ExecutionState&&) = default;

  StatusFlags& Flags() { return flags_; }
  const StatusFlags& Flags() const { return flags_; }

  /// Stack operations

  void PushFlags() { stack_.PushFlags(flags_); }
  void PushAccumulator() { stack_.PushA(a_reg_, flags_); }
  void PushXRegister() { stack_.PushX(x_reg_, flags_); }
  void PushYRegister() { stack_.PushY(y_reg_, flags_); }
  void PullAccumulator() { a_reg_ = stack_.PullA(flags_); }
  void PullXRegister() { x_reg_ = stack_.PullX(flags_); }
  void PullYRegister() { y_reg_ = stack_.PullY(flags_); }

  void PullFlags() {
    StackValue value = stack_.PullByte();
    // TODO: We could update flags from a literal value.  I don't see a reason
    // to do it, nor do I see a reason not to do it.
    if (value.type() != StackValue::T_flags) {
      // Oops, our status flags just got clobbered (but we retain the E bit)
      flags_ = StatusFlags(flags_.EBit());
    } else {
      // The `e` bit isn't pushed on the stack, so we retain that, but update
      // all the others.
      StatusFlags new_flags = value.flags();
      flags_ = StatusFlags(flags_.EBit(), new_flags.MBit(), new_flags.XBit(),
                           new_flags.CBit());
    }
  }

  void WipeAccumulator() { a_reg_ = RegisterValue(); }
  void WipeCarry() { flags_.SetCBit(B_unknown); }

  RegisterValue& Accumulator() { return a_reg_; }
  const RegisterValue& Accumulator() const { return a_reg_; }

  RegisterValue& XRegister() { return x_reg_; }
  const RegisterValue& XRegister() const { return x_reg_; }

  RegisterValue& YRegister() { return y_reg_; }
  const RegisterValue& YRegister() const { return y_reg_; }

  RegisterValue& DataBankRegister() { return dbr_; }
  const RegisterValue& DataBankRegister() const { return dbr_; }

  Stack& GetStack() { return stack_; }
  const Stack& GetStack() const { return stack_; }

  ExecutionState& operator|=(const ExecutionState& rhs) {
    a_reg_ |= rhs.a_reg_;
    x_reg_ |= rhs.x_reg_;
    y_reg_ |= rhs.y_reg_;
    dbr_ |= rhs.dbr_;
    flags_ |= rhs.flags_;
    stack_ |= rhs.stack_;
    return *this;
  }

  ExecutionState operator|(const ExecutionState& rhs) const {
    ExecutionState result = *this;
    return result |= rhs;
  }

  bool operator==(const ExecutionState& rhs) const {
    return a_reg_ == rhs.a_reg_ && x_reg_ == rhs.x_reg_ &&
           y_reg_ == rhs.y_reg_ && dbr_ == rhs.dbr_ && flags_ == rhs.flags_ &&
           stack_ == rhs.stack_;
  }

  bool operator!=(const ExecutionState& rhs) const { return !(*this == rhs); }

 private:
  RegisterValue a_reg_;
  RegisterValue x_reg_;
  RegisterValue y_reg_;
  RegisterValue dbr_;
  StatusFlags flags_;
  Stack stack_;
};

}  // namespace nsasm

#endif  // NSASM_EXECUTION_STATE_H_
