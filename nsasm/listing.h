#ifndef NSASM_LISTING_H_
#define NSASM_LISTING_H_

#include <string>

#include "nsasm/disassemble.h"

namespace nsasm {

// Renders a disassembly as .asm listing text, with one instruction per line,
// annotated with address and mode comments.  This is the format emitted by
// the disassembly command-line utilities.
std::string FormatDisassembly(const DisassemblyMap& disassembly);

}  // namespace nsasm

#endif  // NSASM_LISTING_H_
