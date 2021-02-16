#include "mappers/ppu_map.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void PPUMap::write(AddressT addr, DataT data) {
  if (addr < 0x2000 && cart_.chrRamSize) {
    uint32_t bank = (cart_.chrBankSelect) * 0x2000;
    cart_.chrRam[bank + addr] = data;
  } else if (addr < 0x3F00) {
    addr = mirror_vram_addr(addr);
    nametable_[addr] = data;
  } else {
    palette_[addr & 0x1F] = data;
  }
}

PPUMap::DataT PPUMap::read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    uint32_t bank = (cart_.chrBankSelect) * 0x2000;
    if (cart_.chrRamSize) {
      result = cart_.chrRam[bank + addr];
    } else {
      result = cart_.chrRom[bank + addr];
    }
  } else if (addr < 0x3F00) {
    addr = mirror_vram_addr(addr);
    result = nametable_[addr];
  } else {
    result = palette_[addr & 0x1F];
  }
  return result;
}

PPUMap::AddressT PPUMap::mirror_vram_addr(AddressT addr) {
  uint16_t result = addr & 0x7FF;
  if (cart_.ignoreMirror) {
    // TODO(oren): really this implies some other ram somewhere
    // not yet implemented. here we just mask down into the size of the
    // nametable
    return result;
  }
  // nametable select
  uint8_t nt = (addr & 0x0C00) >> 10;
  // offset within nametable
  uint16_t off = addr & 0x03FF;

  if (cart_.mirroring == 0) { // horizontal
    switch (nt) {
    case 0:
    case 1:
      result = off;
      break;
    case 2:
    case 3:
      result = 0x400 | off;
      break;
    }
  } else { // vertical
    switch (nt) {
    case 0:
    case 2:
      result = off;
      break;
    case 1:
    case 3:
      result = 0x400 | off;
      break;
    }
  }
  return result;
}

} // namespace mapper
