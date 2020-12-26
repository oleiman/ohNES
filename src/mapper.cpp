#include "mapper.hpp"

#include <iostream>
#include <sstream>

using CName = ppu::Registers::CName;

namespace mapper {
void NROM::write(AddressT addr, DataT data) {
  if (addr < 0x2000) {
    internal_[addr & 0x7FF] = data;
  } else if (addr < 0x4000) {
    // std::cerr << "writing to ppu reg" << std::endl;
    ppu_reg_.write(CName(addr & 0x07), data);
  } else if (addr < 0x4020) {
    // APU and I/O registers
  } else if (addr < 0x6000) {
    // god only knows...
    // apparently rarely used
  } else if (addr < 0x8000) {
    // cartridge VRAM/VROM
    // I believe PPU 0x0000 - 0x2000 maps here
    if (cart_.hasPrgRam) {
      cart_.prg_ram_[addr & (cart_.prgRamSize - 1)] = data;
    }
  } else { // 0x8000 <= addr < 0x10000
    // cartridge rom, also not allowed, I assume
    // presumably most existing roms won't break the rules, but
    // we should probably panic here for debugging purposes
    // cart_.prg_rom_[addr & (cart_.prgRomSize - 1)] = data;
    std::stringstream ss;
    ss << "WRITE TO CART ROM 0x" << std::hex << addr;
    // std::cerr << ss.str() << std::endl;
    throw std::runtime_error(ss.str());
    // cart_.prg_rom_[addr & (cart_.prgRomSize - 1)] = data;
    // "ATTEMPT TO WRITE TO CART ROM " << std::hex << addr << std::endl;
  }
}

NROM::DataT NROM::read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    result = internal_[addr & 0x7FF];
  } else if (addr < 0x4000) {
    // std::cerr << "reading from ppu reg " << std::hex << +addr << std::endl;
    // TODO(oren): I think it's safe to assume that we'll need some special
    // logic here, pending my learning what these actually do LOL

    result = ppu_reg_.read(CName(addr & 0x07));
    // break 0xc7af
    // if (addr == 0x2002) {
    //   std::cout << +CName(addr & 0x07) << " " << +result << std::endl;
    // }

  } else if (addr < 0x4020) {
    // APU and I/O
  } else if (addr < 0x6000) {
    // rarely used?
  } else if (addr < 0x8000) {
    std::cerr << "reading from prg ram" << std::endl;
    if (cart_.hasPrgRam) {
      result = cart_.prg_ram_[addr & (cart_.prgRamSize - 1)];
    }
  } else {
    result = cart_.prg_rom_[addr & (cart_.prgRomSize - 1)];
  }
  return result;
}

void PPUMap::write(AddressT addr, DataT data) {
  if (addr < 0x2000) {
    // TODO(oren): illegal if this pattern table is in cartridge ROM,
    // but it might be CHRRAM? how does this work?
  } else if (addr < 0x3F00) {
    // NOTE(oren): not sure whether or when this normally happens
    addr = mirror_vram_addr(addr);
    cart_.chr_rom_[addr] = data;
  } else {
    palette_[addr & 0x1F] = data;
  }
}

PPUMap::DataT PPUMap::read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    // NOTE(oren): this is the pattern table. it should always be *somewhere*...
    if (cart_.chrRomSize > 0) {
      result = cart_.chr_rom_[addr];
    }
  } else if (addr < 0x3F00) {
    addr = mirror_vram_addr(addr);
    result = nametable_[addr];
  } else {
    // TODO(oren): this should be accessible from the register object
    // or from the ppu or whatever
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
