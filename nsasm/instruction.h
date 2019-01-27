#ifndef NSASM_INSTRUCTION_H_
#define NSASM_INSTRUCTION_H_

#include "nsasm/addressing_mode.h"
#include "nsasm/expression.h"
#include "nsasm/mnemonic.h"

namespace nsasm {

struct Instruction {
  Mnemonic mnemonic;
  AddressingMode addressing_mode;
  ExpressionOrNull arg1;
  ExpressionOrNull arg2;

  std::string ToString() const;
};

}  // namespace nsasm

#endif  // NSASM_INSTRUCTION_H_
