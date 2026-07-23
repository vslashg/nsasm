#include "test/test_rom.h"

#include <map>

#include "absl/strings/str_cat.h"
#include "nsasm/file.h"

namespace nsasm {

namespace {

// Collects (SNES address, byte) pairs from assembly, for later conversion
// into a ROM image.
class RecordingSink : public OutputSink {
 public:
  ErrorOr<void> Write(nsasm::Address address,
                      absl::Span<const std::uint8_t> data) override {
    for (std::uint8_t byte : data) {
      received_[address] = byte;
      address = address.AddWrapped(1);
    }
    return {};
  }

  const std::map<Address, std::uint8_t>& Received() const { return received_; }

 private:
  std::map<Address, std::uint8_t> received_;
};

// Builds a zero-filled ROM image large enough to hold every mapped byte, and
// places the bytes.
ErrorOr<std::unique_ptr<Rom>> MakeRomFromByteMap(
    Mapping mapping, const std::map<Address, std::uint8_t>& byte_map,
    std::string path) {
  std::map<size_t, std::uint8_t> rom_offset_map;
  size_t max_offset = 0;
  for (const auto& [address, byte] : byte_map) {
    auto offset = SnesToROMAddress(address, mapping);
    NSASM_RETURN_IF_ERROR(offset);
    rom_offset_map[*offset] = byte;
    max_offset = std::max(max_offset, *offset);
  }
  // Round the image size up to a 0x8000 multiple (one LoROM bank).
  size_t size = ((max_offset / 0x8000) + 1) * 0x8000;
  std::vector<std::uint8_t> data(size, 0);
  for (const auto& [offset, byte] : rom_offset_map) {
    data[offset] = byte;
  }
  return std::make_unique<Rom>(mapping, std::move(path), std::vector<uint8_t>(),
                               std::move(data));
}

}  // namespace

ErrorOr<std::unique_ptr<Rom>> MakeTestRom(
    Mapping mapping, const std::vector<RomContents>& contents,
    std::string path) {
  std::map<Address, std::uint8_t> byte_map;
  for (const RomContents& run : contents) {
    Address address = run.address;
    for (std::uint8_t byte : run.bytes) {
      byte_map[address] = byte;
      address = address.AddWrapped(1);
    }
  }
  return MakeRomFromByteMap(mapping, byte_map, std::move(path));
}

ErrorOr<AssembledRom> AssembleToRom(
    const std::vector<std::string_view>& asm_contents, Mapping mapping) {
  RecordingSink sink;
  std::vector<File> files;
  for (size_t i = 0; i < asm_contents.size(); ++i) {
    files.push_back(
        MakeFakeFile(absl::StrCat("fake_file_", i, ".asm"), asm_contents[i]));
  }
  auto assembler = Assemble(files, &sink);
  NSASM_RETURN_IF_ERROR(assembler);

  auto rom = MakeRomFromByteMap(mapping, sink.Received(), "test.rom");
  NSASM_RETURN_IF_ERROR(rom);
  return AssembledRom{*std::move(rom), *std::move(assembler)};
}

}  // namespace nsasm
