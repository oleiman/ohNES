#include "mappers/color_dreams.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

constexpr uint32_t PRG_BANK_SIZE = 0x8000;
constexpr uint32_t CHR_BANK_SIZE = 0x2000;

namespace mapper {

void ColorDreams::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000) {
    // no prg ram here
    assert(false);
  } else {
    prgBankSelect = data & 0b11;
    chrBankSelect = (data >> 4) & 0b1111;
  }
}
ColorDreams::DataT ColorDreams::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    assert(false);
  } else {
    auto bank = PRG_BANK_SIZE * prgBankSelect;
    auto offset = addr & (PRG_BANK_SIZE - 1);
    return cart_.prgRom[bank + offset];
  }
}
void ColorDreams::chrWrite(AddressT addr, DataT data) {
  assert(false);
  // no chr ram
}

ColorDreams::DataT ColorDreams::chrRead(AddressT addr) {
  auto bank = CHR_BANK_SIZE * chrBankSelect;
  auto offset = addr & (CHR_BANK_SIZE - 1);
  return cart_.chrRom[bank + offset];
}
} // namespace mapper
