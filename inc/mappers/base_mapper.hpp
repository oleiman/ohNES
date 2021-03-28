#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {
class NESMapper : public mem::Mapper {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;
  virtual ~NESMapper() = default;
  virtual void write(AddressT, DataT) = 0;
  virtual DataT read(AddressT) = 0;
  virtual void ppu_write(AddressT, DataT) = 0;
  virtual DataT ppu_read(AddressT) = 0;
};

template <class Derived> class NESMapperBase : public NESMapper {
public:
  using CName = vid::Registers::CName;

protected:
  cart::Cartridge const &cart_;
  std::array<DataT, 0x800> internal_{};
  vid::Registers &ppu_reg_;
  std::array<DataT, 0x100> &ppu_oam_;
  std::array<DataT, 0x800> nametable_{};
  std::array<DataT, 32> palette_;
  ctrl::JoyPad &joypad_;
  uint8_t prgBankSelect = 0;
  uint8_t chrBankSelect = 0;

public:
  NESMapperBase(cart::Cartridge const &c, vid::Registers &reg,
                std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : cart_(c), ppu_reg_(reg), ppu_oam_(oam), joypad_(pad) {}
  virtual ~NESMapperBase() = default;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8);

  // TODO(oren): magic numbers
  void write(AddressT addr, DataT data) {
    if (addr < 0x2000) {
      internal_[addr & 0x7FF] = data;
    } else if (addr < 0x4000) {
      ppu_reg_.write(CName(addr & 0x07), data);
    } else if (addr == 0x4014) {
      oamDma(static_cast<AddressT>(data) << 8);
    } else if (addr == 0x4016) {
      joypad_.toggleStrobe();
    } else if (addr == 0x4017) {
      // TODO(oren): controller 2
    } else if (addr < 0x4020) {
      // TODO(oren): APU and more I/O
    } else if (addr < 0x6000) {
      // TODO(oren); not yet implemented, rarely used, see docs
    } else {
      // TODO(oren): Cartridge space (varies by mapper)
      static_cast<Derived *>(this)->cartWrite(addr, data);
    }
  }
  DataT read(AddressT addr) {
    if (addr < 0x2000) {
      return internal_[addr & 0x7FF];
    } else if (addr < 0x4000) {
      return ppu_reg_.read(CName(addr & 0x07));
    } else if (addr == 0x4016) {
      return joypad_.readNext();
    } else if (addr == 0x4017) {
      // TODO(oren): controller 2
      return 0;
    } else if (addr < 0x4020) {
      // TODO(oren): APU and more I/O
      return 0;
    } else if (addr < 0x6000) {
      // TODO(OREN): rarely used, see docs
      return 0;
    } else {
      // TODO(oren): Cartridge space (varies by mapper)
      return static_cast<Derived *>(this)->cartRead(addr);
    }
  }
  void ppu_write(AddressT addr, DataT data) {
    if (addr < 0x2000 && cart_.chrRamSize) {
      static_cast<Derived *>(this)->chrWrite(addr, data);
    } else if (addr < 0x3F00) {
      addr = mirror_vram_addr(addr, cart_.mirroring);
      nametable_[addr] = data;
    } else {
      palette_[addr & 0x1F] = data;
    }
  }
  DataT ppu_read(AddressT addr) {
    if (addr < 0x2000) {
      return static_cast<Derived *>(this)->chrRead(addr);
    } else if (addr < 0x3F00) {
      addr = mirror_vram_addr(addr, cart_.mirroring);
      return nametable_[addr];
    } else {
      return palette_[addr & 0x1F];
    }
  }

protected:
  void oamDma(AddressT base) {
    uint8_t oam_base = ppu_reg_.oamAddr();
    for (int i = 0; i < ppu_oam_.size(); ++i) {
      ppu_oam_[oam_base + i] = internal_[base | i];
    }
    ppu_reg_.signalOamDma();
  }

  // TODO(oren): cleanup and add single-screen case (type trait??)
  virtual AddressT mirror_vram_addr(AddressT addr, uint8_t mirroring) {
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
}; // namespace mapper
} // namespace mapper
