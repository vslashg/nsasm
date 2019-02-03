#ifndef NSASM_NUMERIC_TYPE_H
#define NSASM_NUMERIC_TYPE_H

#include <string>

namespace nsasm {

// 65816 numeric representation type
enum NumericType {
  T_unknown,
  T_byte,         // 8 bit, unsigned
  T_word,         // 16 bit, unsigned
  T_long,         // 24 bit, unsigned
  T_signed_byte,  // 8 bit, signed
  T_signed_word,  // 16 bit, signed
  T_signed_long,  // 24 bit, signed
};

// Return `value`, coerced to the specified type
inline int CastTo(NumericType type, int value) {
  switch (type) {
    case T_byte:
      return value & 0xff;
    case T_word:
      return value & 0xffff;
    case T_long:
      return value & 0xffffff;
    case T_signed_byte: {
      int v = value & 0xff;
      return (v >= 0x80) ? v - 0x100 : v;
    }
    case T_signed_word: {
      int v = value & 0xffff;
      return (v >= 0x8000) ? v - 0x10000 : v;
    }
    case T_signed_long: {
      int v = value & 0xffffff;
      return (v >= 0x800000) ? v - 0x1000000 : v;
    }
    default:
      return value;
  }
}

inline NumericType Unsigned(NumericType type) {
  if (type == T_signed_byte) return T_byte;
  if (type == T_signed_word) return T_word;
  if (type == T_signed_long) return T_long;
  return type;
}

inline NumericType Signed(NumericType type) {
  if (type == T_byte) return T_signed_byte;
  if (type == T_word) return T_signed_word;
  if (type == T_long) return T_signed_long;
  return type;
}

inline bool IsSigned(NumericType type) {
  return type == T_signed_byte || type == T_signed_word ||
         type == T_signed_long;
}

// The type to use when two types are combined (by +, say)
inline NumericType ArtihmeticConversion(NumericType lhs, NumericType rhs) {
  if (lhs == T_unknown) return rhs;
  if (rhs == T_unknown) return lhs;
  bool signed_result = IsSigned(lhs) || IsSigned(rhs);
  NumericType wider = std::max(Unsigned(lhs), Unsigned(rhs));
  return signed_result ? Signed(wider) : Unsigned(wider);
}

inline std::string ToString(NumericType type) {
  if (type == T_unknown) return "unknown";
  if (type == T_byte) return "byte";
  if (type == T_word) return "word";
  if (type == T_long) return "long";
  if (type == T_signed_byte) return "signed byte";
  if (type == T_signed_word) return "signed word";
  if (type == T_signed_long) return "signed long";
  return "???";
}

}  // namespace nsasm

#endif  // NSASM_NUMERIC_TYPE_H