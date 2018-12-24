#ifndef NSASM_INSTRUCTION_H_
#define NSASM_INSTRUCTION_H_

#include "nsasm/addressing_mode.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

struct Instruction {
  Mnemonic mnemonic;
  AddressingMode addressing_mode;
  int arg1;
  int arg2;
};

}  // namespace nsasm

#endif  // NSASM_INSTRUCTION_H_
