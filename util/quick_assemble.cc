#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "nsasm/assembler.h"
#include "nsasm/rom.h"

// Test utility to exercise assembly

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom-file> <path-to-output> {<path-to-asm-file> "
      "...}\n\n"
      "Assembles one or more ASM files, or returns an error message.\n"
      "If path-to-output is `-`, instead check that the asm files make no \n"
      "changes to the ROM being overwritten.",
      path);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    usage(argv[0]);
    return 0;
  }
  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("Error loading ROM: %s\n", rom.error().ToString());
    return 1;
  }

  std::string output_path = argv[2];
  bool identity_test = (output_path == std::string("-"));
  if (absl::EndsWith(output_path, ".asm")) {
    absl::PrintF("Error: %s given as output path\n", output_path);
    return 1;
  }

  std::unique_ptr<nsasm::OutputSink> sink;
  if (identity_test) {
    sink = absl::make_unique<nsasm::RomIdentityTest>(std::move(*rom));
  } else {
    sink = absl::make_unique<nsasm::RomOverwriter>(std::move(*rom));
  }

  std::vector<nsasm::File> asm_files;
  for (int arg_index = 3; arg_index < argc; ++arg_index) {
    auto file = nsasm::OpenFile(argv[arg_index]);
    if (!file.ok()) {
      absl::PrintF("Error loading file: %s\n", file.error().ToString());
      return 1;
    }
    asm_files.push_back(*std::move(file));
  }

  auto assembler = nsasm::Assemble(asm_files, sink.get());
  if (!assembler.ok()) {
    absl::PrintF("Error assembling: %s\n", assembler.error().ToString());
    return 1;
  }

  if (identity_test) {
    auto jump_targets = assembler->JumpTargets();
    absl::PrintF("%d jump targets found\n", jump_targets.size());
    for (const auto& node : jump_targets) {
      absl::PrintF("  %s %s\n", node.first.ToString(), node.second.ToString());
    }
  } else {
    const auto& overwriter = dynamic_cast<const nsasm::RomOverwriter&>(*sink);
    auto write_status = overwriter.CreateFile(output_path);
    if (!write_status.ok()) {
      absl::PrintF("Error writing file: %s\n", write_status.error().ToString());
    }
  }
  // assembler.DebugPrint();
}