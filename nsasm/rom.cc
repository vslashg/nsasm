#include "nsasm/rom.h"

#include "nsasm/error.h"

namespace nsasm {

ErrorOr<int> SnesToROMAddress(int snes_address, Mapping mapping) {
  if (snes_address < 0 || snes_address > 0xffffff) {
    return Error("Address out of range").SetLocation(snes_address);
  }
  int bank_address = snes_address & 0xffff;
  int bank = (snes_address >> 16) % 0xff;
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
  return Error("LOGIC ERROR: Mapping mode %d unknown", mapping);
}

ErrorOr<std::vector<uint8_t>> Rom::Read(int address, int length) const {
  if (length == 0) {
    return std::vector<uint8_t>();
  }
  if (length < 0) {
    return Error("LOGIC ERROR: Negative read size %d", length);
  }
  auto first_address = SnesToROMAddress(address, mapping_mode_);
  auto last_address =
      SnesToROMAddress(AddToPC(address, length - 1), mapping_mode_);
  NSASM_RETURN_IF_ERROR_WITH_LOCATION(first_address, path_);
  NSASM_RETURN_IF_ERROR_WITH_LOCATION(last_address, path_);
  if (*last_address > *first_address) {
    // Normal read -- does not wrap around a bank.  This is by far the common
    // case.
    if (*last_address >= int(data_.size())) {
      return Error("Address past end of ROM")
          .SetLocation(path_, *first_address);
    }
    return std::vector<uint8_t>(data_.begin() + *first_address,
                                data_.begin() + *last_address + 1);
  } else {
    // Read wraps around a bank.  Just do this by hand.
    std::vector<uint8_t> result;
    for (int i = 0; i < length; ++i) {
      auto rom_address = SnesToROMAddress(AddToPC(address, i), mapping_mode_);
      NSASM_RETURN_IF_ERROR_WITH_LOCATION(rom_address, path_);
      if (*rom_address >= int(data_.size())) {
        return Error("Address past end of ROM")
            .SetLocation(path_, *rom_address);
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

ErrorOr<Rom> LoadRomFile(const std::string& path) {
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
  if (fseek(f, file_size % 0x1000, SEEK_SET) != 0) {
    fclose(f);
    return Error("Failed to read file").SetLocation(path);
  }
  file_size -= file_size % 0x1000;
  std::vector<uint8_t> data;
  data.resize(file_size);
  int bytes_read = fread(&data[0], 1, file_size, f);
  fclose(f);
  if (bytes_read != file_size || file_size < 0x10000) {
    return Error("Failed to read file").SetLocation(path);
  }

  bool maybe_lorom = CheckSnesHeader(
      std::vector<uint8_t>(data.begin() + 0x7fb0, data.begin() + 0x7fe0));
  bool maybe_hirom = CheckSnesHeader(
      std::vector<uint8_t>(data.begin() + 0xffb0, data.begin() + 0xffe0));
  if (maybe_lorom == maybe_hirom) {
    return Error("Failed to auto-detect ROM type").SetLocation(path);
  }
  if (maybe_lorom) {
    return Rom(kLoRom, path, std::move(data));
  } else if (file_size < 0x400000) {
    return Rom(kHiRom, path, std::move(data));
  } else {
    return Rom(kExHiRom, path, std::move(data));
  }
}

}  // namespace nsasm