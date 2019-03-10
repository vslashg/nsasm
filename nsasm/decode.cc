#include "nsasm/decode.h"

#include "absl/memory/memory.h"
#include "nsasm/expression.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

ErrorOr<Instruction> Decode(absl::Span<const uint8_t> bytes,
                            const FlagState& state) {
  if (bytes.empty()) {
    return Error("Not enough bytes to decode");
  }
  uint8_t opcode = bytes.front();
  bytes.remove_prefix(1);

  Instruction decoded;
  std::tie(decoded.mnemonic, decoded.addressing_mode) = DecodeOpcode(opcode);

  // correct for sentinel addressing modes
  if (decoded.addressing_mode == A_imm_fm ||
      decoded.addressing_mode == A_imm_fx) {
    BitState narrow_register =
        (decoded.addressing_mode == A_imm_fm) ? state.MBit() : state.XBit();
    if (narrow_register == B_on) {
      decoded.addressing_mode = A_imm_b;
    } else if (narrow_register == B_off) {
      decoded.addressing_mode = A_imm_w;
    } else {
      return Error(
          "Argument size of opcode 0x%02x (%s) depends on processor state, "
          "which is not known here",
          opcode, ToString(decoded.mnemonic));
    }
  }

  // Read arguments
  if (decoded.addressing_mode == A_dir_l ||
      decoded.addressing_mode == A_dir_lx) {
    // 24 bit argument
    if (bytes.size() < 3) {
      return Error("Not enough bytes to decode");
    }
    decoded.arg1 = absl::make_unique<Literal>(
        bytes[0] + (bytes[1] * 256) + (bytes[2] * 256 * 256), T_long);
  }
  if (decoded.addressing_mode == A_imm_w ||
      decoded.addressing_mode == A_dir_w ||
      decoded.addressing_mode == A_dir_wx ||
      decoded.addressing_mode == A_dir_wy ||
      decoded.addressing_mode == A_ind_w ||
      decoded.addressing_mode == A_ind_wx ||
      decoded.addressing_mode == A_lng_w) {
    // 12 bit argument
    if (bytes.size() < 2) {
      return Error("Not enough bytes to decode");
    }
    decoded.arg1 =
        absl::make_unique<Literal>(bytes[0] + (bytes[1] * 256), T_word);
  }
  if (decoded.addressing_mode == A_imm_b ||
      decoded.addressing_mode == A_dir_b ||
      decoded.addressing_mode == A_dir_bx ||
      decoded.addressing_mode == A_dir_by ||
      decoded.addressing_mode == A_ind_b ||
      decoded.addressing_mode == A_ind_bx ||
      decoded.addressing_mode == A_ind_by ||
      decoded.addressing_mode == A_lng_b ||
      decoded.addressing_mode == A_lng_by || decoded.addressing_mode == A_stk ||
      decoded.addressing_mode == A_stk_y) {
    // 8 bit argument
    if (bytes.size() < 1) {
      return Error("Not enough bytes to decode");
    }
    decoded.arg1 = absl::make_unique<Literal>(bytes[0], T_byte);
  }
  if (decoded.addressing_mode == A_mov) {
    // pair of 8 bit arguments
    if (bytes.size() < 2) {
      return Error("Not enough bytes to decode");
    }
    decoded.arg1 = absl::make_unique<Literal>(bytes[0], T_byte);
    decoded.arg2 = absl::make_unique<Literal>(bytes[1], T_byte);
  }
  if (decoded.addressing_mode == A_rel8) {
    // 8 bit signed argument
    if (bytes.size() < 1) {
      return Error("Not enough bytes to decode");
    }
    int value = bytes[0];
    if (value >= 128) {
      value -= 256;
    }
    decoded.arg1 = absl::make_unique<Literal>(value, T_signed_byte);
  }
  if (decoded.addressing_mode == A_rel16) {
    // 8 bit signed argument
    if (bytes.size() < 2) {
      return Error("Not enough bytes to decode");
    }
    int value = bytes[0] + (bytes[1] * 256);
    if (value >= 32768) {
      value -= 65536;
    }
    decoded.arg1 = absl::make_unique<Literal>(value, T_signed_word);
  }
  return {std::move(decoded)};
}

}  // namespace nsasm
