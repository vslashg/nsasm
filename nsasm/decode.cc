#include "nsasm/decode.h"

#include "nsasm/argument.h"
#include "nsasm/opcode_map.h"

namespace nsasm {

absl::optional<Instruction> Decode(absl::Span<const uint8_t> bytes,
                                   const FlagState& state) {
  if (bytes.empty()) {
    return absl::nullopt;
  }
  uint8_t opcode = bytes.front();
  bytes.remove_prefix(1);

  Instruction decoded = DecodeOpcode(opcode);

  // correct for sentinel addressing modes
  if (decoded.addressing_mode == A_imm_fm || decoded.addressing_mode == A_imm_fx) {
    BitState narrow_register =
        (decoded.addressing_mode == A_imm_fm) ? state.MBit() : state.XBit();
    if (narrow_register == B_on) {
      decoded.addressing_mode = A_imm_b;
    } else if (narrow_register == B_off) {
      decoded.addressing_mode = A_imm_w;
    } else {
      return absl::nullopt;
    }
  }

  // Read arguments
  if (decoded.addressing_mode == A_dir_l ||
      decoded.addressing_mode == A_dir_lx) {
    // 24 bit argument
    if (bytes.size() < 3) {
      return absl::nullopt;
    }
    decoded.arg1 =
        Argument(bytes[0] + (bytes[1] * 256) + (bytes[2] * 256 * 256));
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
      return absl::nullopt;
    }
    decoded.arg1 = Argument(bytes[0] + (bytes[1] * 256));
  }
  if (decoded.addressing_mode == A_imm_b ||
      decoded.addressing_mode == A_dir_b ||
      decoded.addressing_mode == A_dir_bx ||
      decoded.addressing_mode == A_dir_by ||
      decoded.addressing_mode == A_ind_b ||
      decoded.addressing_mode == A_ind_bx ||
      decoded.addressing_mode == A_ind_by ||
      decoded.addressing_mode == A_lng_b ||
      decoded.addressing_mode == A_lng_by ||
      decoded.addressing_mode == A_stk ||
      decoded.addressing_mode == A_stk_y) {
    // 8 bit argument
    if (bytes.size() < 1) {
      return absl::nullopt;
    }
    decoded.arg1 = Argument(bytes[0]);
  }
  if (decoded.addressing_mode == A_mov) {
    // pair of 8 bit arguments
    if (bytes.size() < 2) {
      return absl::nullopt;
    }
    decoded.arg1 = Argument(bytes[0]);
    decoded.arg2 = Argument(bytes[1]);
  }
  if (decoded.addressing_mode == A_rel8) {
    // 8 bit signed argument
    if (bytes.size() < 1) {
      return absl::nullopt;
    }
    int value = bytes[0];
    if (value >= 128) {
      value -= 256;
    }
    decoded.arg1 = Argument(value);
  }
  if (decoded.addressing_mode == A_rel16) {
    // 8 bit signed argument
    if (bytes.size() < 2) {
      return absl::nullopt;
    }
    int value = bytes[0] + (bytes[1] * 256);
    if (value >= 32768) {
      value -= 65536;
    }
    decoded.arg1 = Argument(value);
  }
  return decoded;
}

}