#include "mapper.hpp"

#include <iostream>
#include <sstream>

namespace mapper {
void NROM::write(AddressT addr, DataT data) {
  if (addr < 0x2000) {
    internal_[addr & 0x7FF] = data;
  } else if (addr < 0x4000) {
    // TODO(oren): I think it's safe to assume that we'll need some special
    // logic here, pending my learning what these actually do LOL
    ppu_reg_[addr & 0x08] = data;
  } else if (addr < 0x4020) {
    // APU and I/O registers
  } else if (addr < 0x6000) {
    // god only knows...
    // apparently rarely used
  } else if (addr < 0x8000) {
    // cartridge VRAM/VROM
    // I believe PPU 0x0000 - 0x2000 maps here
    if (cart_.hasPrgRam) {
      cart_.prg_ram_[addr & (cart_.prgRamSize - 1)] = data;
    }
  } else { // 0x8000 <= addr < 0x10000
    // cartridge rom, also not allowed, I assume
    // presumably most existing roms won't break the rules, but
    // we should probably panic here for debugging purposes
    // cart_.prg_rom_[addr & (cart_.prgRomSize - 1)] = data;
    std::cerr << "ATTEMPT TO WRITE TO CART ROM" << std::endl;
  }
}

NROM::DataT NROM::read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    result = internal_[addr & 0x7FF];
  } else if (addr < 0x4000) {
    // TODO(oren): I think it's safe to assume that we'll need some special
    // logic here, pending my learning what these actually do LOL
    result = ppu_reg_[addr & 0x08];
  } else if (addr < 0x4020) {
    // APU and I/O
  } else if (addr < 0x6000) {
    // rarely used?
  } else if (addr < 0x8000) {
    if (cart_.hasPrgRam) {
      result = cart_.prg_ram_[addr & (cart_.prgRamSize - 1)];
    }
  } else {
    result = cart_.prg_rom_[addr & (cart_.prgRomSize - 1)];
  }
  return result;
}

void PPUMap::write(AddressT addr, DataT data) {}

PPUMap::DataT PPUMap::read(AddressT addr) { return 0; }

} // namespace mapper
