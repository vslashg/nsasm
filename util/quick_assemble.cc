#include "absl/strings/str_format.h"
#include "nsasm/module.h"
#include "nsasm/rom.h"

// Test utility to exercise assembly

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom-file> <path-to-asm-file>\n\n"
      "\"Assembles\" a single ASM file, or returns an error message.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc != 3) {
    usage(argv[0]);
    return 0;
  }
  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("Error loading ROM: %s\n", rom.error().ToString());
    return 1;
  }
  auto module = nsasm::Module::LoadAsmFile(argv[2]);
  if (!module.ok()) {
    absl::PrintF("Error assembling: %s\n", module.error().ToString());
    return 0;
  }
  auto first_pass = module->RunFirstPass();
  if (!first_pass.ok()) {
    absl::PrintF("Error in first pass: %s\n", first_pass.error().ToString());
    return 0;
  }

  module->DebugPrint();

  nsasm::RomIdentityTest rom_identity(&*rom);
  auto status = module->Assemble(&rom_identity);
  if (!status.ok()) {
    absl::PrintF("Error in assembly: %s\n", status.error().ToString());
    return 0;
  }
}