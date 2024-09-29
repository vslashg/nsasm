#include "nsasm/rom.h"

#include <memory>

#include "nsasm/error.h"

namespace nsasm {

ErrorOr<size_t> SnesToROMAddress(nsasm::Address snes_address, Mapping mapping) {
  int bank_address = snes_address.BankAddress();
  int bank = snes_address.Bank();
  if (bank == 0x7e || bank == 0x7f) {
    return Error("Address in WRAM").SetLocation(snes_address);
  }
  if (bank_address < 0x8000 &&
      ((bank >= 0x00 && bank < 0x40) || (bank >= 0x80 && bank < 0xc0))) {
    return Error("Address in non-CART memory").SetLocation(snes_address);
  }
  if (mapping == kLoRom) {
    if (bank_address < 0x8000) {
      return Error("Invalid LoRom ROM address").SetLocation(snes_address);
    };
    return (bank_address & 0x7fff) | ((bank & 0x7f) << 15);
  } else if (mapping == kHiRom) {
    return bank_address | ((bank & 0x3f) << 16);
  } else if (mapping == kExHiRom) {
    int result = bank_address | ((bank & 0x3f) << 16);
    // address bit 23 is inverted and used as bit 22 of the CART address
    if ((result & 0x800000) == 0) {
      result |= 0x400000;
    }
    return result;
  }
  return Error("LOGIC ERROR: Mapping mode %d unknown", mapping);
}

ErrorOr<std::vector<uint8_t>> Rom::Read(nsasm::Address address,
                                        int length) const {
  if (length == 0) {
    return std::vector<uint8_t>();
  }
  if (length < 0) {
    return Error("LOGIC ERROR: Negative read size %d", length);
  }
  auto first_address = SnesToROMAddress(address, mapping_mode_);
  auto last_address =
      SnesToROMAddress(address.AddWrapped(length - 1), mapping_mode_);
  NSASM_RETURN_IF_ERROR_WITH_LOCATION(first_address, path_);
  NSASM_RETURN_IF_ERROR_WITH_LOCATION(last_address, path_);
  if (*last_address > *first_address) {
    // Normal read -- does not wrap around a bank.  This is by far the common
    // case.
    if (*last_address >= data_.size()) {
      return Error("Address past end of ROM")
          .SetLocation(path_, *first_address);
    }
    return std::vector<uint8_t>(data_.begin() + *first_address,
                                data_.begin() + *last_address + 1);
  } else {
    // Read wraps around a bank.  Just do this by hand.
    std::vector<uint8_t> result;
    for (int i = 0; i < length; ++i) {
      auto rom_address = SnesToROMAddress(address.AddWrapped(i), mapping_mode_);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(rom_address, path_);
      if (*rom_address >= data_.size()) {
        return Error("Address past end of ROM")
            .SetLocation(path_, address.AddWrapped(i));
      }
      result.push_back(data_[*rom_address]);
    }
    return result;
  }
}

namespace {

// Returns true if, heuristically, this looks like a SNES header.
//
// TODO: This is realy poor.
bool CheckSnesHeader(const std::vector<uint8_t>& header) {
  bool checksum_ok = (header[0x2c] ^ header[0x2e]) == 0xff &&
                     (header[0x2d] ^ header[0x2f]) == 0xff;
  return checksum_ok;
}

}  // namespace

ErrorOr<std::unique_ptr<Rom>> LoadRomFile(const std::string& path) {
  // TODO: RAII this file handle
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    return Error("Failed to open file").SetLocation(path);
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return Error("Failed to read file").SetLocation(path);
  }
  int file_size = ftell(f);
  if (file_size <= 0) {
    fclose(f);
    return Error("Failed to read file").SetLocation(path);
  }
  // A SNES rom is in 0x1000-byte page chunks.  SNES ROM files usually contain a
  // 0x0200-byte header in addition to this.  If neither of these is consistent,
  // the ROM is corrupt.
  if (file_size % 0x1000 != 0 && file_size % 0x1000 != 0x200) {
    fclose(f);
    return Error("File is not an SNES ROM").SetLocation(path);
  }
  // Seek to the beginning of the file (skipping the SMC header if present).
  std::vector<uint8_t> header;
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return Error("Failed to read file").SetLocation(path);
  }
  if (file_size % 0x1000 == 0x200) {
    header.resize(0x200);
    int bytes_read = fread(&header[0], 1, 0x200, f);
    if (bytes_read != 0x200) {
      return Error("Failed to read file").SetLocation(path);
    }
    file_size -= 0x200;
  }
  std::vector<uint8_t> data;
  data.resize(file_size);
  int bytes_read = fread(&data[0], 1, file_size, f);
  fclose(f);
  if (bytes_read != file_size || file_size < 0x10000) {
    return Error("Failed to read file e").SetLocation(path);
  }

  bool maybe_lorom = CheckSnesHeader(
      std::vector<uint8_t>(data.begin() + 0x7fb0, data.begin() + 0x7fe0));
  bool maybe_hirom = CheckSnesHeader(
      std::vector<uint8_t>(data.begin() + 0xffb0, data.begin() + 0xffe0));
  if (maybe_lorom == maybe_hirom) {
    return Error("Failed to auto-detect ROM type").SetLocation(path);
  }
  if (maybe_lorom) {
    return std::make_unique<Rom>(kLoRom, path, std::move(header),
                                 std::move(data));
  } else if (file_size < 0x400000) {
    return std::make_unique<Rom>(kHiRom, path, std::move(header),
                                 std::move(data));
  } else {
    return std::make_unique<Rom>(kExHiRom, path, std::move(header),
                                 std::move(data));
  }
}

ErrorOr<void> RomIdentityTest::Write(nsasm::Address address,
                                     absl::Span<const std::uint8_t> data) {
  auto actual = rom_->Read(address, data.size());
  NSASM_RETURN_IF_ERROR(actual);

  if (*actual == data) {
    return {};
  }

  for (size_t i = 0; i < data.size(); ++i) {
    if (data[i] != (*actual)[i]) {
      return Error("Wrote 0x%02x to %s, expected 0x%02x", data[i],
                   address.AddWrapped(i).ToString(), (*actual)[i]);
    }
  }

  return Error("logic error: comparisons inconsistent??");
}

ErrorOr<void> RomOverwriter::Write(Address address,
                                   absl::Span<const std::uint8_t> data) {
  // just dumbly write; this could be made more efficient, but for now
  // this whole output scheme is meh
  for (size_t i = 0; i < data.size(); ++i) {
    auto rom_index =
        SnesToROMAddress(address.AddWrapped(i), rom_->mapping_mode_);
    NSASM_RETURN_IF_ERROR(rom_index);
    if (*rom_index > data_.size()) {
      return Error("Attempt to write at %s, past end of file",
                   address.AddWrapped(i).ToString());
    }
    data_[*rom_index] = data[i];
  }
  return {};
}

ErrorOr<void> RomOverwriter::CreateFile(const std::string& path) const {
  // TODO: RAII this file handle
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) {
    return Error("Failed to open file for write").SetLocation(path);
  }
  if (!rom_->header_.empty()) {
    size_t written = fwrite(&rom_->header_[0], 1, 0x200, f);
    if (written != 0x200) {
      return Error("Failed to write header").SetLocation(path);
    }
  }
  size_t written = fwrite(&data_[0], 1, data_.size(), f);
  if (written != data_.size()) {
    return Error("Failed to write payload").SetLocation(path);
  }
  if (fclose(f) != 0) {
    return Error("Failed to close file").SetLocation(path);
  }
  return {};
}

}  // namespace nsasm
