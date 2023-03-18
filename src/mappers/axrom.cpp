#include "mappers/axrom.hpp"

#include <iostream>

using CName = vid::Registers::CName;

constexpr uint32_t PRG_BANK_SIZE = 0x8000;
constexpr uint32_t PRG_ADDR_MASK = PRG_BANK_SIZE - 1;
constexpr uint32_t CHR_BANK_SIZE = 0x2000;
constexpr uint32_t CHR_ADDR_MASK = CHR_BANK_SIZE - 1;

namespace mapper {

void AxROM::cartWrite(AddressT addr, DataT data) {
  //
  assert(addr >= 0x8000);

  prgBankSelect = data & 0b111;

  mirror = (data >> 4) & 0b1;
}

AxROM::DataT AxROM::cartRead(AddressT addr) {
  assert(addr >= 0x8000);
  uint32_t bank = prgBankSelect * PRG_BANK_SIZE;
  AddressT offset = addr & PRG_ADDR_MASK;
  return cart_.prgRom[bank + offset];
}

void AxROM::chrWrite(AddressT addr, DataT data) {
  assert(cart_.chrRamSize);
  AddressT offset = addr & CHR_ADDR_MASK;
  cart_.chrRam[offset] = data;
}

AxROM::DataT AxROM::chrRead(AddressT addr) {
  AddressT offset = addr & CHR_ADDR_MASK;
  if (cart_.chrRamSize) {
    return cart_.chrRam[offset];
  } else {
    return cart_.chrRom[offset];
  }
}
} // namespace mapper
