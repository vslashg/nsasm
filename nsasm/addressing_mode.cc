#include "nsasm/addressing_mode.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string ArgsToString(AddressingMode addressing_mode, int arg1, int arg2,
                         bool wide_mode) {
  if (addressing_mode == A_imm_a) {
    addressing_mode = wide_mode ? A_imm_w : A_imm_b;
  }
  switch (addressing_mode) {
    case A_imp:
    case A_acc: {
      return "";
    }
    case A_imm_b: {
      arg1 &= 0xff;
      return absl::StrFormat(" #$%02x", arg1);
    }
    case A_imm_w: {
      arg1 &= 0xffff;
      return absl::StrFormat(" #$%04x", arg1);
    }
    case A_dir_b: {
      arg1 &= 0xff;
      return absl::StrFormat(" $%02x", arg1);
    }
    case A_dir_w: {
      arg1 &= 0xffff;
      return absl::StrFormat(" $%04x", arg1);
    }
    case A_dir_l: {
      arg1 &= 0xffffff;
      return absl::StrFormat(" $%06x", arg1);
    }
    case A_dir_bx: {
      arg1 &= 0xff;
      return absl::StrFormat(" $%02x, X", arg1);
    }
    case A_dir_by: {
      arg1 &= 0xff;
      return absl::StrFormat(" $%02x, Y", arg1);
    }
    case A_dir_wx: {
      arg1 &= 0xffff;
      return absl::StrFormat(" $%04x, X", arg1);
    }
    case A_dir_wy: {
      arg1 &= 0xffff;
      return absl::StrFormat(" $%04x, Y", arg1);
    }
    case A_dir_lx: {
      arg1 &= 0xffffff;
      return absl::StrFormat(" $%06x, X", arg1);
    }
    case A_ind_b: {
      arg1 &= 0xff;
      return absl::StrFormat(" ($%02x)", arg1);
    }
    case A_ind_w: {
      arg1 &= 0xffff;
      return absl::StrFormat(" ($%04x)", arg1);
    }
    case A_ind_bx: {
      arg1 &= 0xff;
      return absl::StrFormat(" ($%02x, X)", arg1);
    }
    case A_ind_by: {
      arg1 &= 0xff;
      return absl::StrFormat(" ($%02x), Y", arg1);
    }
    case A_ind_wx: {
      arg1 &= 0xffff;
      return absl::StrFormat(" ($%04x, X)", arg1);
    }
    case A_lng_b: {
      arg1 &= 0xff;
      return absl::StrFormat(" [$%02x]", arg1);
    }
    case A_lng_w: {
      arg1 &= 0xffff;
      return absl::StrFormat(" [$%04x]", arg1);
    }
    case A_lng_by: {
      arg1 &= 0xff;
      return absl::StrFormat(" [$%02x], Y", arg1);
    }
    case A_stk: {
      arg1 &= 0xff;
      return absl::StrFormat(" $%02x, S", arg1);
    }
    case A_stk_y: {
      arg1 &= 0xff;
      return absl::StrFormat(" ($%02x, S), Y", arg1);
    }
    case A_mov: {
      arg1 &= 0xff;
      arg2 &= 0xff;
      return absl::StrFormat(" #$%02x, #$%02x", arg1, arg2);
    }
    case A_rel8:
    case A_rel16: {
      return absl::StrFormat(" @%d", arg1);
    }
    default:
      return "";
  }
}

}  // namespace nsasm