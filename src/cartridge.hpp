#pragma once

#include <array>
#include <fstream>
#include <vector>

namespace cart {
struct Cartridge {
  Cartridge(std::string_view const &romfile);
  void parseHeader(std::array<uint8_t, 16> hdr);
  uint32_t prgRomSize = 0;
  uint32_t chrRomSize = 0;
  uint32_t chrRamSize = 0;
  // TODO(oren): mnemonic would be nice to have here
  uint8_t mirroring = 0;
  bool hasPrgRam = true;
  bool prgRamBattery = false;
  uint16_t prgRamSize = 0;
  bool hasTrainer = false;
  bool ignoreMirror = false;
  bool vsSystem = false;
  bool pc10 = false;
  bool nes2 = false;

  // NOTE(oren): almost completely unused, apparently
  uint8_t tvSystem = 0;

  uint8_t mapper = 0;

  std::vector<uint8_t> trainer;
  std::vector<uint8_t> prgRom;
  std::vector<uint8_t> chrRom;
  mutable std::vector<uint8_t> chrRam;
  std::vector<uint8_t> pcInstRom;
  std::vector<uint8_t> pcProm;
  mutable std::vector<uint8_t> prgRam;

  const std::string romfile;

  friend std::ostream &operator<<(std::ostream &os, const Cartridge &in);
};

} // namespace cart
