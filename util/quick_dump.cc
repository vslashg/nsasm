#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "nsasm/rom.h"

void usage(char* path) {
  absl::PrintF(
      "Usage: %s <path-to-rom> <snes-hex-address> <count> [stride] [per-line]"
      "\n\n"
      "Emits a literal dump of the memory at the given ROM location.\n"
      "Makes the given number of entries.  Stride may be 1, 2, or 3, to \n"
      "select .db, .dw, or .dl.\n",
      path);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    usage(argv[0]);
    return 0;
  }

  auto rom = nsasm::LoadRomFile(argv[1]);
  if (!rom.ok()) {
    absl::PrintF("%s\n", rom.error().ToString());
    return 1;
  }

  unsigned start;
  int count;
  int stride = 1;
  int per_line = 8;
  if (!sscanf(argv[2], "%x", &start)) {
    usage(argv[0]);
    return 1;
  }
  if (!sscanf(argv[3], "%d", &count)) {
    usage(argv[0]);
    return 1;
  }
  if (argc > 4 && !sscanf(argv[4], "%d", &stride)) {
    usage(argv[0]);
    return 1;
  }
  if (argc > 5 && !sscanf(argv[5], "%d", &per_line)) {
    usage(argv[0]);
    return 1;
  }

  if (stride > 3 || stride < 2) {
    stride = 1;
  }
  if (count < 1) {
    count = 1;
  }
  if (per_line < 1) {
    per_line = 1;
  }

  std::vector<std::string> values;
  nsasm::Address address(start);
  for (int i = 0; i < count; ++i, address = address.AddWrapped(stride)) {
    nsasm::ErrorOr<int> value = 0;
    switch (stride) {
      default:
      case 1:
        value = rom->ReadByte(address);
        break;
      case 2:
        value = rom->ReadWord(address);
        break;
      case 3:
        value = rom->ReadLong(address);
        break;
    }
    if (!value.ok()) {
      absl::PrintF("%s\n", rom.error().ToString());
      return 1;
    }
    switch (stride) {
      default:
      case 1:
        values.push_back(absl::StrFormat("$%02x", *value));
        break;
      case 2:
        values.push_back(absl::StrFormat("$%04x", *value));
        break;
      case 3:
        values.push_back(absl::StrFormat("$%06x", *value));
        break;
    }
  }

  std::string directive = ".db ";
  if (stride == 2) {
    directive = ".dw ";
  } else if (stride == 3) {
    directive = ".dl ";
  }

  absl::PrintF(".org $%06x\n", start);
  while (values.size() > per_line) {
    absl::PrintF(
        "%s %s\n", directive,
        absl::StrJoin(values.begin(), values.begin() + per_line, ", "));
    values.erase(values.begin(), values.begin() + per_line);
  }
  if (!values.empty()) {
    absl::PrintF("%s %s\n", directive, absl::StrJoin(values, ", "));
  }

  return 0;
}
