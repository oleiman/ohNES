#include "mappers/nrom.hpp"

#include <iostream>
#include <sstream>

namespace mapper {

void NROM::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000) {
    // cartridge VRAM/VROM
    if (cart_.hasPrgRam) {
      cart_.prgRam[addr & (cart_.prgRamSize - 1)] = data;
    }
  } else { // 0x8000 <= addr < 0x10000
    // std::stringstream ss;
    // ss << "WRITE TO PRG ROM 0x" << std::hex << addr;
    // std::cerr << ss.str() << std::endl;
    // std::raise(SIGSEGV);
    // throw std::runtime_error(ss.str());

    // NOTE(oren): some tests write to rom deliberately. this seems to be
    // occasionally the case for certain roms, so I don't think there's any
    // reason to report it.
    // TODO(oren): Add some form of message logging more robust than stderr and
    // separate from instruction logging (maybe interspersed?).
  }
}

NROM::DataT NROM::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    if (cart_.hasPrgRam) {
      return cart_.prgRam[addr & (cart_.prgRamSize - 1)];
    } else {
      return 0;
    }
  } else {
    return cart_.prgRom[addr & (cart_.prgRomSize - 1)];
  }
}

void NROM::chrWrite(AddressT addr, DataT data) { cart_.chrRam[addr] = data; }
NROM::DataT NROM::chrRead(AddressT addr) {
  auto &chrMem = (cart_.chrRamSize ? cart_.chrRam : cart_.chrRom);
  return chrMem[addr];
}

} // namespace mapper
