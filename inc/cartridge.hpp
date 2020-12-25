#pragma once

#include <array>
#include <fstream>
#include <vector>

namespace cart {
struct Cartridge {
  Cartridge(std::ifstream &infile);
  void parseHeader(std::array<uint8_t, 16> hdr);
  uint16_t prgRomSize = 0;
  uint16_t chrRomSize = 0;
  // TODO(oren): mnemonic would be nice to have here
  uint8_t mirroring = 0;
  bool hasPrgRam = false;
  uint8_t prgRamSize = 0;
  bool hasTrainer = false;
  bool ignoreMirror = false;
  bool vsSystem = false;
  bool pc10 = false;
  bool nes2 = false;

  // NOTE(oren): almost completely unused, apparently
  uint8_t tvSystem = 0;

  uint8_t mapper = 0;

  std::vector<uint8_t> trainer_;
  std::vector<uint8_t> prg_rom_;
  std::vector<uint8_t> chr_rom_;
  std::vector<uint8_t> pc_inst_rom_;
  std::vector<uint8_t> pc_prom_;
  std::vector<uint8_t> prg_ram_;

  friend std::ostream &operator<<(std::ostream &os, const Cartridge &in);
};

} // namespace cart
