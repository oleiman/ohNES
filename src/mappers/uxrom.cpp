#include "mappers/uxrom.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void UxROM::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000) {
    // TODO(oren): not mapped?
  } else {
    prgBankSelect = data;
  }
}

UxROM::DataT UxROM::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    // TODO(oren): not mapped
    return 0;
  } else if (addr < 0xC000) {
    // switchable PRG ROM bank
    uint32_t bank = (prgBankSelect)*0x4000;
    uint16_t offset = addr & (0x4000 - 1);
    return cart_.prgRom[bank + offset];
  } else { // 0xC000 <= addr <= 0xFFFF
    uint32_t fixed_bank = cart_.prgRomSize - 0x4000;
    uint16_t offset = addr & (0x4000 - 1);
    return cart_.prgRom[fixed_bank + offset];
  }
}

void UxROM::chrWrite(AddressT addr, DataT data) { cart_.chrRam[addr] = data; }
UxROM::DataT UxROM::chrRead(AddressT addr) {
  auto &chrMem = (cart_.chrRamSize ? cart_.chrRam : cart_.chrRom);
  return chrMem[addr];
}

} // namespace mapper
