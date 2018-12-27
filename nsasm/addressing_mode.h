#ifndef NSASM_ADDRESSING_MODE_H_
#define NSASM_ADDRESSING_MODE_H_

#include <string>

#include "nsasm/argument.h"

namespace nsasm {

enum AddressingMode : int {
  A_imp,     //            Implied (0 bytes)
  A_acc,     // A or ''    Accumulator (0 bytes)
  A_imm_b,   // #$12       Immediate fixed byte (1 byte) (REP/SEP/COP)
  A_imm_w,   // #$1234     Immediate fixed word (2 bytes) (PEA)
  A_dir_b,   // $12        Direct page direct (1 byte)
  A_dir_w,   // $1234      Absolute direct (2 bytes)
  A_dir_l,   // $123456    Absolute long direct (3 bytes)
  A_dir_bx,  // $12,X      Direct page indexed with X (1 byte)
  A_dir_by,  // $12,Y      Direct page indexed with Y (1 byte)
  A_dir_wx,  // $1234,X    Absolute indexed with X (2 bytes)
  A_dir_wy,  // $1234,Y    Absolute indexed with Y (2 bytes)
  A_dir_lx,  // $123456,X  Absolute long indexed with X (3 bytes)
  A_ind_b,   // ($12)      Direct page indirect (1 byte)
  A_ind_w,   // ($1234)    Absolute indirect (2 bytes)
  A_ind_bx,  // ($12,X)    Direct page indexed indrect with X (1 byte)
  A_ind_by,  // ($12),Y    Direct page indirect indexed with Y (1 byte)
  A_ind_wx,  // ($1234,X)  Absolute indexed indirect with X (2 bytes)
  A_lng_b,   // [$12]      Direct page indirect long (1 byte)
  A_lng_w,   // [$1234]    Absolute indrect long (2 bytes)
  A_lng_by,  // [$12],Y    Direct page indirect long indexed with Y (1 byte)
  A_stk,     // $12,S      Stack relative (1 byte)
  A_stk_y,   // ($12,S),Y  Stack relative indirect indexed with Y (1 byte)
  A_mov,     // #$12,#$34  Source Destination (1 byte, 1 byte)
  A_rel8,    // label      Relative 8 (1 byte) (BEQ, etc.)
  A_rel16,   // label      Relative 16 (2 bytes) (BRL/PER)

  // sentinel values, indicating an addressing mode dependent on processor flags
  A_imm_fm,  // #$12..     Immediate fixed word (size based on m flag) (ADC)
  A_imm_fx,  // #$12..     Immediate fixed word (size based on x flag) (LDX)
};

// Renders an argument list that can be appended to an instruction mnemonic.
std::string ArgsToString(AddressingMode a, Argument arg1, Argument arg2);

// Returns the size of an instruction with the given addressing mode.
//
// Returns -1 on invalid input.
int InstructionLength(AddressingMode a);

}  // namespace nsasm

#endif  // NSASM_ADDRESSING_MODE_H_