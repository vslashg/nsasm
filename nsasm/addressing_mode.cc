#include "nsasm/addressing_mode.h"

#include "absl/strings/str_format.h"

namespace nsasm {

std::string ArgsToString(AddressingMode addressing_mode, Argument arg1,
                         Argument arg2) {
  switch (addressing_mode) {
    case A_imp:
    case A_acc:
    default: {
      return "";
    }
    case A_imm_b: {
      return absl::StrFormat(" #%s", arg1.ToString(1));
    }
    case A_imm_w: {
      return absl::StrFormat(" #%s", arg1.ToString(2));
    }
    case A_dir_b: {
      return absl::StrFormat(" %s", arg1.ToString(1));
    }
    case A_dir_w: {
      return absl::StrFormat(" %s", arg1.ToString(2));
    }
    case A_dir_l: {
      return absl::StrFormat(" %s", arg1.ToString(3));
    }
    case A_dir_bx: {
      return absl::StrFormat(" %s, X", arg1.ToString(1));
    }
    case A_dir_by: {
      return absl::StrFormat(" %s, Y", arg1.ToString(1));
    }
    case A_dir_wx: {
      return absl::StrFormat(" %s, X", arg1.ToString(2));
    }
    case A_dir_wy: {
      return absl::StrFormat(" %s, Y", arg1.ToString(2));
    }
    case A_dir_lx: {
      return absl::StrFormat(" %s, X", arg1.ToString(3));
    }
    case A_ind_b: {
      return absl::StrFormat(" (%s)", arg1.ToString(1));
    }
    case A_ind_w: {
      return absl::StrFormat(" (%s)", arg1.ToString(2));
    }
    case A_ind_bx: {
      return absl::StrFormat(" (%s, X)", arg1.ToString(1));
    }
    case A_ind_by: {
      return absl::StrFormat(" (%s), Y", arg1.ToString(1));
    }
    case A_ind_wx: {
      return absl::StrFormat(" (%s, X)", arg1.ToString(2));
    }
    case A_lng_b: {
      return absl::StrFormat(" [%s]", arg1.ToString(1));
    }
    case A_lng_w: {
      return absl::StrFormat(" [%s]", arg1.ToString(2));
    }
    case A_lng_by: {
      return absl::StrFormat(" [%s], Y", arg1.ToString(1));
    }
    case A_stk: {
      return absl::StrFormat(" %s, S", arg1.ToString(1));
    }
    case A_stk_y: {
      return absl::StrFormat(" (%s, S), Y", arg1.ToString(1));
    }
    case A_mov: {
      return absl::StrFormat(" #%s, #%s", arg1.ToString(1), arg2.ToString(1));
    }
    case A_rel8:
    case A_rel16: {
      return absl::StrFormat(" %s", arg1.ToBranchOffset());
    }
  }
}

int InstructionLength(AddressingMode a) {
  if (a == A_imp || a == A_acc) {
    return 1;
  }
  if (a == A_imm_b || a == A_dir_b || a == A_dir_bx || a == A_dir_by ||
      a == A_ind_b || a == A_ind_bx || a == A_ind_by || a == A_lng_b ||
      a == A_lng_by || a == A_stk || a == A_stk_y || a == A_rel8) {
    return 2;
  }
  if (a == A_imm_w || a == A_dir_w || a == A_dir_wx || a == A_dir_wy ||
      a == A_ind_w || a == A_ind_wx || a == A_lng_w || a == A_mov ||
      a == A_rel16) {
    return 3;
  }
  if (a == A_dir_l || a == A_dir_lx) {
    return 4;
  }
  return -1;
}

}  // namespace nsasm