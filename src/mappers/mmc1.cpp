#include "mappers/mmc1.hpp"

using CName = vid::Registers::CName;

namespace mapper {

void MMC1::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000 && cart_.hasPrgRam && prgRamEnable_) {
    cart_.prgRam[addr & 0x1FFF] = data;
  } else {
    // this is where the magic happens...
    // TODO(oren): general quality lol
    if (data & 0b10000000) {
      shiftRegister_ = 0b00010000;
    } else {
      uint8_t in = (data & 0x01) << 5;
      shiftRegister_ |= in;
      bool full = shiftRegister_ & 0x01;
      shiftRegister_ >>= 1;
      if (full) {
        if (addr < 0xA000) {
          // control register
          mirrorMode_ = MirrorMap[shiftRegister_ & 0b11];
          shiftRegister_ >>= 2;
          prgMode_ = PrgMap[shiftRegister_ & 0b11];
          shiftRegister_ >>= 2;
          chrMode_ = ChrMap[shiftRegister_ = 0b1];
        } else if (addr < 0xC000) {
          // chr bank 0
          chrBankSelect = shiftRegister_;
        } else if (addr < 0xE000) {
          chrBankSelect2_ = shiftRegister_;
          // chr bank 1
        } else {
          prgBankSelect = shiftRegister_ & 0b1111;
          shiftRegister_ >>= 4;
          prgRamEnable_ = !shiftRegister_;
        }
        shiftRegister_ = 0b00010000;
      }
    }
  }
}

MMC1::DataT MMC1::cartRead(AddressT addr) {
  if (addr < 0x8000 && cart_.hasPrgRam && prgRamEnable_) {
    return cart_.prgRam[addr & 0x1FFF];
  } else if (prgMode_ == SWITCH32) {
    uint32_t bank = (prgBankSelect >> 1) * 0x8000;
    uint16_t offset = addr & 0x7FFF;
    return cart_.prgRom[bank + offset];
  } else if (addr < 0xC000) {
    if (prgMode_ == FIXFIRST) {
      return cart_.prgRom[addr & 0x3FFF];
    } else {
      uint32_t bank = prgBankSelect * 0x4000;
      uint16_t offset = addr & 0x3FFF;
      return cart_.prgRom[bank + offset];
    }
  } else {
    if (prgMode_ == FIXLAST) {
      uint32_t bank = cart_.prgRomSize - 0x4000;
      uint16_t offset = addr & 0x3FFF;
      return cart_.prgRom[bank + offset];
    } else {
      uint32_t bank = prgBankSelect * 0x4000;
      uint16_t offset = addr & 0x3FFF;
      return cart_.prgRom[bank + offset];
    }
  }
}

void MMC1::chrWrite(AddressT addr, DataT data) {
  if (cart_.chrRamSize) {
    if (chrMode_ == SWITCH8) {
      uint32_t bank = (chrBankSelect >> 1) * 0x2000;
      cart_.chrRam[bank + addr] = data;
    } else if (addr < 0x1000) {
      uint32_t bank = chrBankSelect * 0x1000;
      uint16_t offset = addr & 0x0FFF;
      cart_.chrRam[bank + offset] = data;
    } else {
      uint32_t bank = chrBankSelect2_ * 0x1000;
      uint16_t offset = addr & 0x0FFF;
      cart_.chrRam[bank + offset] = data;
    }
  }
}

MMC1::DataT MMC1::chrRead(AddressT addr) {
  uint32_t bank = (chrBankSelect >> 1) * 0x2000;
  uint16_t offset = addr;
  if (chrMode_ == SWITCH2x4) {
    if (addr < 0x1000) {
      bank = chrBankSelect * 0x1000;
      offset = addr & 0x0FFF;
    } else {
      bank = chrBankSelect2_ * 0x1000;
      offset = addr & 0x0FFF;
    }
  }
  auto &chrMem = (cart_.chrRamSize ? cart_.chrRam : cart_.chrRom);
  return chrMem[bank + offset];
}

MMC1::AddressT MMC1::mirror_vram_addr(AddressT addr, uint8_t mirroring) {
  if (cart_.ignoreMirror) {
    return addr & (nametable_.size() - 1);
  } else {
    uint16_t sz = nametable_.size();
    uint16_t off = addr & ((sz / 1) - 1);
    switch (mirroring) {
    case ONELOWER:
      return off;
    case ONEUPPER:
      return off | (sz / 2);
    case VERTICAL:
      return NESMapperBase<MMC1>::mirror_vram_addr(addr, 1);
    case HORIZONTAL:
      return NESMapperBase<MMC1>::mirror_vram_addr(addr, 0);
    default:
      throw std::runtime_error("Invalid Mirror Mode");
    }
  }
}

const std::array<MMC1::MirrorMode, 4> MMC1::MirrorMap = {ONELOWER, ONEUPPER,
                                                         VERTICAL, HORIZONTAL};
const std::array<MMC1::PrgRomBankMode, 4> MMC1::PrgMap = {SWITCH32, SWITCH32,
                                                          FIXFIRST, FIXLAST};
const std::array<MMC1::ChrRomBankMode, 2> MMC1::ChrMap = {SWITCH8, SWITCH2x4};

} // namespace mapper
