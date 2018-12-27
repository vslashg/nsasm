#include "nsasm/rom.h"

namespace nsasm {

absl::optional<int> SnesToROMAddress(int snes_address, Mapping mapping) {
  if (snes_address < 0 || snes_address > 0xffffff) {
    return absl::nullopt;
  }
  int bank_address = snes_address & 0xffff;
  int bank = (snes_address >> 16) % 0xff;
  if (bank == 0x7e || bank == 0x7f) {
    return absl::nullopt;  // WRAM address banks
  }
  if (bank_address < 0x8000 &&
      ((bank >= 0x00 && bank < 0x40) || (bank >= 0x80 && bank < 0xc0))) {
    return absl::nullopt;  // non-CART address range
  }
  if (mapping == kLoRom) {
    if (bank_address < 0x8000) {
      return absl::nullopt;  // ROM addresses begin at 0x8000 in LoRom
    };
    return (bank_address & 0x7fff) | (bank << 15);
  } else if (mapping == kHiRom) {
    return snes_address & 0x3fffff;
  } else if (mapping == kExHiRom) {
    int result = snes_address & 0x3fffff;
    // address bit 23 is inverted and used as bit 22 of the CART address
    if ((snes_address & 0x800000) == 0) {
      result |= 0x400000;
    }
    return result;
  }
  return absl::nullopt;
}

absl::optional<std::vector<uint8_t>> Rom::Read(int address, int length) {
  if (length == 0) {
    return std::vector<uint8_t>();
  }
  if (length < 0) {
    return absl::nullopt;
  }
  auto first_address = SnesToROMAddress(address, mapping_mode_);
  auto last_address =
      SnesToROMAddress(AddToPC(address, length - 1), mapping_mode_);
  if (!first_address.has_value() || !last_address.has_value()) {
    return absl::nullopt;
  }
  if (*last_address > *first_address) {
    // Normal read -- does not wrap around a bank.  This is by far the common
    // case.
    if (last_address >= data_.size()) {
      return absl::nullopt;  // read past end of ROM
    }
    return std::vector<uint8_t>(data_.begin() + *first_address,
                                data_.begin() + *last_address + 1);
  } else {
    // Read wraps around a bank.  Just do this by hand.
    std::vector<uint8_t> result;
    for (int i = 0; i < length; ++i) {
      auto rom_address = SnesToROMAddress(AddToPC(address, i), mapping_mode_);
      if (!rom_address.has_value() || *rom_address >= data_.size()) {
        return absl::nullopt;
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

absl::optional<Rom> LoadRomFile(const std::string& path) {
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) {
    return absl::nullopt;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return absl::nullopt;
  }
  int file_size = ftell(f);
  if (file_size <= 0) {
    fclose(f);
    return absl::nullopt;
  }
  // A SNES rom is in 0x1000-byte page chunks.  SNES ROM files usually contain a
  // 0x0200-byte header in addition to this.  If neither of these is consistent,
  // the ROM is corrupt.
  if (file_size % 0x1000 != 0 && file_size % 0x1000 != 0x200) {
    fclose(f);
    return absl::nullopt;
  }
  // Seek to the beginning of the file (skipping the SMC header if present).
  if (fseek(f, file_size % 0x1000, SEEK_SET) != 0) {
    fclose(f);
    return absl::nullopt;
  }
  file_size -= file_size % 0x1000;
  std::vector<uint8_t> data;
  data.resize(file_size);
  int bytes_read = fread(&data[0], 1, file_size, f);
  fclose(f);
  if (bytes_read != file_size || file_size < 0x10000) {
    return absl::nullopt;
  }

  bool maybe_lorom = CheckSnesHeader(
      std::vector<uint8_t>(data.begin() + 0x7fb0, data.begin() + 0x7fe0));
  bool maybe_hirom = CheckSnesHeader(
      std::vector<uint8_t>(data.begin() + 0xffb0, data.begin() + 0xffe0));
  if (maybe_lorom == maybe_hirom) {
    return absl::nullopt;
  }
  if (maybe_lorom) {
    return Rom(kLoRom, std::move(data));
  } else if (file_size < 0x400000) {
    return Rom(kHiRom, std::move(data));
  } else {
    return Rom(kExHiRom, std::move(data));
  }
}

}  // namespace nsasm