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
    std::stringstream ss;
    ss << "WRITE TO CART ROM 0x" << std::hex << addr;
    throw std::runtime_error(ss.str());
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
