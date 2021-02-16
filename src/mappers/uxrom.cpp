#include "mappers/uxrom.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void UxROM::write(AddressT addr, DataT data) {
  if (addr < 0x2000) {
    internal_[addr & 0x7FF] = data;
  } else if (addr < 0x4000) {
    ppu_reg_.write(CName(addr & 0x07), data);
  } else if (addr == 0x4014) {
    BaseNESMapper::oamDma(static_cast<AddressT>(data) << 8);
  } else if (addr == 0x4016) {
    // controller 1
    joypad_.toggleStrobe();
  } else if (addr == 0x4017) {
    // controller 2
  } else if (addr < 0x4020) {
    // APU and I/O registers
  } else if (addr < 0x8000) {
    // rarely used?
  } else { // if (addr < 0xC000) {
    // write to bank switch register
    cart_.prgBankSelect = data;
  }
}

UxROM::DataT UxROM::read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    result = internal_[addr & 0x7FF];
  } else if (addr < 0x4000) {
    result = ppu_reg_.read(CName(addr & 0x07));
  } else if (addr == 0x4016) {
    result = joypad_.readNext();

  } else if (addr == 0x4017) {
    // controller 2

  } else if (addr < 0x8000) {
    // rarely used
  } else if (addr < 0xC000) {
    // switchable PRG ROM bank
    uint32_t bank = (cart_.prgBankSelect) * 0x4000;
    uint16_t offset = addr & (0x4000 - 1);
    result = cart_.prgRom[bank + offset];
  } else { // 0xC000 <= addr <= 0xFFFF
    uint32_t fixed_bank = cart_.prgRomSize - 0x4000;
    uint16_t offset = addr & (0x4000 - 1);
    result = cart_.prgRom[fixed_bank + offset];
  }
  return result;
}
} // namespace mapper
