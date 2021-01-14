#include "mapper.hpp"

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
    AddressT base = static_cast<AddressT>(data) << 8;
    uint8_t oam_base = ppu_reg_.oamAddr();
    for (int i = 0; i < ppu_oam_.size(); ++i) {
      ppu_oam_[oam_base + i] = internal_[base | i];
    }
    ppu_reg_.signalOamDma();
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
      cart_.prg_ram_[addr & (cart_.prgRamSize - 1)] = data;
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
      result = cart_.prg_ram_[addr & (cart_.prgRamSize - 1)];
    }
  } else {
    result = cart_.prg_rom_[addr & (cart_.prgRomSize - 1)];
  }
  return result;
} // namespace mapper

void PPUMap::write(AddressT addr, DataT data) {
  if (addr < 0x2000) {
    // TODO(oren): pretty sure this is illegal
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
    if (cart_.chrRomSize > 0) {
      result = cart_.chr_rom_[addr];
    } // else ???
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
