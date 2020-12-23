#pragma once

#include <array>
#include <fstream>
#include <vector>

namespace cart {
struct Cartridge {
  Cartridge(std::ifstream &infile);
  void parseHeader(std::array<uint8_t, 16> hdr);
  uint16_t prgRomSize;
  uint16_t chrRomSize;
  bool hasChrRom;
  uint8_t mirroring;
  bool hasPrgRam;
  uint8_t prgRamSize;
  bool hasTrainer;
  bool ignoreMirror;
  bool vsSystem;
  bool pc10;
  bool nes2;

  // NOTE(oren): almost completely unused, apparently
  uint8_t tvSystem;

  uint8_t mapper;

  std::vector<uint8_t> trainer_;
  std::vector<uint8_t> prg_rom_;
  std::vector<uint8_t> chr_rom_;
  std::vector<uint8_t> pc_inst_rom_;
  std::vector<uint8_t> pc_prom_;
  std::vector<uint8_t> prg_ram_;

  friend std::ostream &operator<<(std::ostream &os, const Cartridge &in);
};

} // namespace cart
