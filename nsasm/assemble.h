#ifndef NSASM_ASSEMBLE_H_
#define NSASM_ASSEMBLE_H_

#include "nsasm/error.h"
#include "nsasm/instruction.h"
#include "nsasm/token.h"

namespace nsasm {

// Assemble an Instruction from the provided span of tokens.
//
// On success, consumes the assembled tokens off the front of the input.
ErrorOr<Instruction> Assemble(absl::Span<const Token>* tokens);

}  // namespace nsasm

#endif  // NSASM_ASSEMBLE_H_