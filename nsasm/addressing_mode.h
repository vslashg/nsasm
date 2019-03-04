#ifndef NSASM_ADDRESSING_MODE_H_
#define NSASM_ADDRESSING_MODE_H_

#include <string>

#include "nsasm/error.h"
#include "nsasm/expression.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

enum AddressingMode {
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

// Syntactic forms of addressing; the actual mode selected depends on mnemonic
// and argument type.
enum SyntacticAddressingMode {
  SA_imp,    //            no arguments
  SA_acc,    // A          literal A
  SA_imm,    // #exp       immediate value
  SA_dir,    // exp        direct value or relative label
  SA_dir_x,  // exp,X      X indexed
  SA_dir_y,  // exp,Y      Y indexed
  SA_ind,    // (exp)      indirect
  SA_ind_x,  // (exp,X)    X indexed indirect
  SA_ind_y,  // (exp),Y    Y indirect indexed
  SA_lng,    // [exp]      indirect long
  SA_lng_y,  // [exp],Y    indirect long indexed
  SA_stk,    // exp,S      stack relative
  SA_stk_y,  // (exp,S),Y  stack relative indirect indexed
  SA_mov,    // #exp,#exp  source/destination
};

// Renders an argument list that can be appended to an instruction mnemonic.
std::string ArgsToString(AddressingMode a, const Expression& arg1,
                         const Expression& arg2);

// Given a syntactic addressing form and arguments, returns the actual
// addressing mode if one can be inferred.  Will return A_imm_fm and A_imm_fx
// for status-flag-dependent immediate arguments.
ErrorOr<AddressingMode> DeduceMode(Mnemonic m, SyntacticAddressingMode smode,
                                   const Expression& arg1,
                                   const Expression& arg2);

// Returns the size of an instruction with the given addressing mode.
//
// Returns -1 on invalid input.
int InstructionLength(AddressingMode a);

// Stringize addressing mode names.  There is no reverse operation; this is
// intended for test code
std::string ToString(AddressingMode a);
std::string ToString(SyntacticAddressingMode s);

// Return all addressing modes, for test code
const std::vector<AddressingMode>& AllAddressingModes();

}  // namespace nsasm

#endif  // NSASM_ADDRESSING_MODE_H_