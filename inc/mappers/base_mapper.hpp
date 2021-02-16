#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {
class BaseNESMapper : public mem::Mapper {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;
  BaseNESMapper(cart::Cartridge const &c, vid::Registers &reg,
                std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : cart_(c), ppu_reg_(reg), ppu_oam_(oam), joypad_(pad) {}
  virtual ~BaseNESMapper() = default;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8);
  virtual void write(AddressT addr, DataT data) = 0;
  virtual DataT read(AddressT addr) = 0;

protected:
  void oamDma(AddressT base);
  cart::Cartridge const &cart_;
  std::array<DataT, 0x800> internal_{};
  vid::Registers &ppu_reg_;
  std::array<DataT, 0x100> &ppu_oam_;
  ctrl::JoyPad &joypad_;
};
} // namespace mapper
