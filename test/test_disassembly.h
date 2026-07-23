#ifndef TEST_TEST_DISASSEMBLY_H_
#define TEST_TEST_DISASSEMBLY_H_

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "nsasm/calling_convention.h"
#include "nsasm/disassemble.h"
#include "nsasm/error.h"
#include "nsasm/memory.h"

namespace nsasm {

// Runs the disassembler over the given seeds to fixpoint (following
// discovered far jump targets), then returns the formatted listing text.
// Unlike the command-line drivers, any per-seed error fails the whole run.
ErrorOr<std::string> DisassembleToString(
    std::unique_ptr<InputSource> src, std::map<Address, StatusFlags> seeds,
    const std::map<Address, ReturnConvention>& conventions = {});

// Test that disassembling from the given seeds yields exactly the expected
// listing text.  On mismatch, the failure message contains both listings in
// full, for easy expectation regeneration.
void ExpectDisassembly(
    std::unique_ptr<InputSource> src, std::map<Address, StatusFlags> seeds,
    std::string_view expected_listing,
    const std::map<Address, ReturnConvention>& conventions = {});

// Test that disassembling from the given seeds fails, with `message` (if
// provided) as a substring of the error.
void ExpectDisassemblyError(
    std::unique_ptr<InputSource> src, std::map<Address, StatusFlags> seeds,
    std::string_view message = "",
    const std::map<Address, ReturnConvention>& conventions = {});

}  // namespace nsasm

#endif  // TEST_TEST_DISASSEMBLY_H_
