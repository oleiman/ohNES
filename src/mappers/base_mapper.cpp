// TODO(oren): REMOVE FILE

#include "mappers/base_mapper.hpp"

namespace mapper {
void BaseNESMapperBasic::oamDma(AddressT base) {
  uint8_t oam_base = ppu_reg_.oamAddr();
  for (int i = 0; i < ppu_oam_.size(); ++i) {
    ppu_oam_[oam_base + i] = internal_[base | i];
  }
  ppu_reg_.signalOamDma();
}

void BaseNESMapperBasic::ppu_write(AddressT addr, DataT data) {
  if (addr < 0x2000 && cart_.chrRamSize) {
    uint32_t bank = (chrBankSelect)*0x2000;
    cart_.chrRam[bank + addr] = data;
  } else if (addr < 0x3F00) {
    addr = mirror_vram_addr(addr, cart_.mirroring);
    nametable_[addr] = data;
  } else {
    palette_[addr & 0x1F] = data;
  }
}

BaseNESMapperBasic::DataT BaseNESMapperBasic::ppu_read(AddressT addr) {
  DataT result = 0x00;
  if (addr < 0x2000) {
    uint32_t bank = (chrBankSelect)*0x2000;
    if (cart_.chrRamSize) {
      result = cart_.chrRam[bank + addr];
    } else {
      result = cart_.chrRom[bank + addr];
    }
  } else if (addr < 0x3F00) {
    addr = mirror_vram_addr(addr, cart_.mirroring);
    result = nametable_[addr];
  } else {
    result = palette_[addr & 0x1F];
  }
  return result;
}

BaseNESMapperBasic::AddressT
BaseNESMapperBasic::mirror_vram_addr(AddressT addr, uint8_t mirroring) {
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

  if (mirroring == 0) { // horizontal
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
