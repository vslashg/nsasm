#include "absl/strings/str_format.h"
#include "nsasm/module.h"

// Test utility to exercise assembly

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-asm-file>\n\n"
      "\"Assembles\" a single ASM file, or returns an error message.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    usage(argv[0]);
    return 0;
  }
  auto assembled = nsasm::Module::LoadAsmFile(argv[1]);
  if (!assembled.ok()) {
    absl::PrintF("Error assembling: %s\n", assembled.error().ToString());
    return 0;
  }
  auto first_pass = assembled->RunFirstPass();
  if (!first_pass.ok()) {
    absl::PrintF("Error in first pass: %s\n", first_pass.error().ToString());
    return 0;
  }

  assembled->DebugPrint();
}