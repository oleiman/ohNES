#include "mappers/nrom.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void NROM::write(AddressT addr, DataT data) {
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
  } else if (addr < 0x6000) {
    // god only knows...
    // apparently rarely used
  } else if (addr < 0x8000) {
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

NROM::DataT NROM::read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    result = internal_[addr & 0x7FF];
  } else if (addr < 0x4000) {
    result = ppu_reg_.read(CName(addr & 0x07));
  } else if (addr == 0x4016) {
    result = joypad_.readNext();
  } else if (addr == 0x4017) {
    // controller 2
  } else if (addr < 0x4020) {
    // APU and I/O
  } else if (addr < 0x6000) {
    // rarely used?
  } else if (addr < 0x8000) {
    std::cerr << "reading from prg ram" << std::endl;
    if (cart_.hasPrgRam) {
      result = cart_.prgRam[addr & (cart_.prgRamSize - 1)];
    }
  } else {
    result = cart_.prgRom[addr & (cart_.prgRomSize - 1)];
  }
  return result;
}
} // namespace mapper
