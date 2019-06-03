#include "absl/strings/str_format.h"
#include "nsasm/assembler.h"
#include "nsasm/rom.h"

// Test utility to exercise assembly

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom-file> {<path-to-asm-file> ...}\n\n"
      "\"Assembles\" one or more ASM files, or returns an error message.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    usage(argv[0]);
    return 0;
  }
  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("Error loading ROM: %s\n", rom.error().ToString());
    return 1;
  }
  nsasm::Assembler assembler;
  for (int arg_index = 2; arg_index < argc; ++arg_index) {
    auto result = assembler.AddAsmFile(argv[arg_index]);
    if (!result.ok()) {
      absl::PrintF("Error assembling: %s\n", result.error().ToString());
    }
  }

  nsasm::RomIdentityTest rom_identity(&*rom);
  auto status = assembler.Assemble(&rom_identity);
  if (!status.ok()) {
    absl::PrintF("Error in assembly: %s\n", status.error().ToString());
    return 0;
  }

  //assembler.DebugPrint();
}