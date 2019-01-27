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

inline std::string ToString(NumericType type) {
  if (type == T_unknown) return "unknown";
  if (type == T_byte) return "byte";
  if (type == T_word) return "word";
  if (type == T_long) return "long";
  if (type == T_signed_byte) return "signed byte";
  if (type == T_signed_word) return "signed word";
  if (type == T_signed_long) return "signed long";
}

}  // namespace nsasm

#endif  // NSASM_NUMERIC_TYPE_H