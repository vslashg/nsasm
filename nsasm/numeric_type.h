#ifndef NSASM_NUMERIC_TYPE_H
#define NSASM_NUMERIC_TYPE_H

namespace nsasm {

// 65816 numeric representation type
enum NumericType {
  N_unknown,
  N_byte,         // 8 bit, unsigned
  N_word,         // 16 bit, unsigned
  N_long,         // 24 bit, unsigned
  N_signed_byte,  // 8 bit, signed
  N_signed_word,  // 16 bit, signed
  N_signed_long,  // 24 bit, signed
};

// Return `value`, coerced to the specified type
inline int CastTo(NumericType type, int value) {
  switch (type) {
    case N_byte:
      return value & 0xff;
    case N_word:
      return value & 0xffff;
    case N_long:
      return value & 0xffffff;
    case N_signed_byte: {
      int v = value & 0xff;
      return (v >= 0x80) ? v - 0x100 : v;
    }
    case N_signed_word: {
      int v = value & 0xffff;
      return (v >= 0x8000) ? v - 0x10000 : v;
    }
    case N_signed_long: {
      int v = value & 0xffffff;
      return (v >= 0x800000) ? v - 0x1000000 : v;
    }
    default:
      return value;
  }
}

}  // namespace nsasm

#endif  // NSASM_NUMERIC_TYPE_H