#include "mappers/cnrom.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void CNROM::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000) {
    // NOTE(oren): CNROM hardware is not specified with any PRG RAM; however,
    // some test roms might, so we map addresses in [0x6000, 0x8000) in the most
    // straightforward way possible. Example: blargg's ppu_read_buffer test
    // suite.
    if (cart_.hasPrgRam) {
      const AddressT PRG_RAM_MASK = cart_.prgRamSize - 1;
      cart_.prgRam[addr & PRG_RAM_MASK] = data;
    }
  } else { // 0x8000 <= addr < 0x10000
    chrBankSelect = data & 0b11;
  }
}

CNROM::DataT CNROM::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    if (cart_.hasPrgRam) {
      const AddressT PRG_RAM_MASK = cart_.prgRamSize - 1;
      return cart_.prgRam[addr & PRG_RAM_MASK];
    } else {
      return 0;
    }
  } else {
    return cart_.prgRom[addr & (cart_.prgRomSize - 1)];
  }
}

void CNROM::chrWrite(AddressT addr, DataT data) {
  uint32_t bank = (chrBankSelect)*0x2000;
  cart_.chrRam[bank + addr] = data;
}

CNROM::DataT CNROM::chrRead(AddressT addr) {
  auto &chrMem = (cart_.chrRamSize ? cart_.chrRam : cart_.chrRom);
  uint32_t bank = (chrBankSelect)*0x2000;
  return chrMem[bank + addr];
}
} // namespace mapper
