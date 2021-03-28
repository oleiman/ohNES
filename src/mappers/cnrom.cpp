#include "mappers/cnrom.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void CNROM::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000) {
    // No Prg RAM available for this cartridge type
  } else { // 0x8000 <= addr < 0x10000
    chrBankSelect = data;
  }
}

CNROM::DataT CNROM::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    return 0;
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
