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
  nsasm::RomIdentityTest rom_identity_sink(&*rom);

  std::vector<nsasm::File> asm_files;
  for (int arg_index = 2; arg_index < argc; ++arg_index) {
    auto file = nsasm::OpenFile(argv[arg_index]);
    if (!file.ok()) {
      absl::PrintF("Error loading file: %s\n", file.error().ToString());
      return 1;
    }
    asm_files.push_back(*std::move(file));
  }

  auto assembler = nsasm::Assemble(asm_files, &rom_identity_sink);
  if (!assembler.ok()) {
    absl::PrintF("Error assembling: %s\n", assembler.error().ToString());
    return 1;
  }

  auto jump_targets = assembler->JumpTargets();
  absl::PrintF("%d jump targets found\n", jump_targets.size());
  for (const auto& node : jump_targets) {
    absl::PrintF("  %s %s\n", node.first.ToString(), node.second.ToString());
  }
  // assembler.DebugPrint();
}