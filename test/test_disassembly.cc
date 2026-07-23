#include "test/test_disassembly.h"

#include "absl/strings/match.h"
#include "gtest/gtest.h"
#include "nsasm/listing.h"

namespace nsasm {

namespace {

// Merges newly discovered jump targets into the seed set for the next pass.
// (Same combining logic as the command-line drivers.)
void CombineStates(std::map<Address, StatusFlags>* targets,
                   const std::map<Address, StatusFlags>& new_targets) {
  for (const auto& node : new_targets) {
    auto it = targets->find(node.first);
    if (it == targets->end()) {
      (*targets)[node.first] = node.second;
    } else {
      it->second |= node.second;
    }
  }
}

ErrorOr<Disassembler> RunToFixpoint(
    std::unique_ptr<InputSource> src, std::map<Address, StatusFlags> seeds,
    const std::map<Address, ReturnConvention>& conventions) {
  Disassembler disassembler(std::move(src));
  disassembler.AddTargetReturnConventions(conventions);
  for (int pass = 0; pass < 100 && !seeds.empty(); ++pass) {
    std::map<Address, StatusFlags> new_seeds;
    for (const auto& node : seeds) {
      auto branch_targets = disassembler.Disassemble(node.first, node.second);
      NSASM_RETURN_IF_ERROR(branch_targets);
      CombineStates(&new_seeds, *branch_targets);
    }
    seeds = std::move(new_seeds);
  }
  NSASM_RETURN_IF_ERROR(disassembler.Cleanup());
  return {std::move(disassembler)};
}

}  // namespace

ErrorOr<std::string> DisassembleToString(
    std::unique_ptr<InputSource> src, std::map<Address, StatusFlags> seeds,
    const std::map<Address, ReturnConvention>& conventions) {
  auto disassembler =
      RunToFixpoint(std::move(src), std::move(seeds), conventions);
  NSASM_RETURN_IF_ERROR(disassembler);
  return FormatDisassembly(disassembler->Result());
}

void ExpectDisassembly(std::unique_ptr<InputSource> src,
                       std::map<Address, StatusFlags> seeds,
                       std::string_view expected_listing,
                       const std::map<Address, ReturnConvention>& conventions) {
  auto listing = DisassembleToString(std::move(src), std::move(seeds),
                                     conventions);
  if (!listing.ok()) {
    ADD_FAILURE() << listing.error().ToString();
    return;
  }
  if (*listing != expected_listing) {
    ADD_FAILURE() << "Disassembly mismatch.\nExpected:\n"
                  << expected_listing << "\nActual:\n"
                  << *listing;
  }
}

void ExpectDisassemblyError(
    std::unique_ptr<InputSource> src, std::map<Address, StatusFlags> seeds,
    std::string_view message,
    const std::map<Address, ReturnConvention>& conventions) {
  auto listing = DisassembleToString(std::move(src), std::move(seeds),
                                     conventions);
  if (listing.ok()) {
    ADD_FAILURE() << "Unexpected successful disassembly:\n" << *listing;
    return;
  }
  if (!absl::StrContains(listing.error().ToString(), message)) {
    ADD_FAILURE() << "Expected `" << message << "` in error message, got: "
                  << listing.error().ToString();
  }
}

}  // namespace nsasm
