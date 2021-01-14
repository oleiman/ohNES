#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {
class NROM : public mem::Mapper {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;

  explicit NROM(cart::Cartridge &c, vid::Registers &reg,
                std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : cart_(c), ppu_reg_(reg), ppu_oam_(oam), joypad_(pad) {}
  ~NROM() = default;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8);
  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  // 2KB of internal RAM
  cart::Cartridge &cart_;
  std::array<DataT, 0x800> internal_{};
  vid::Registers &ppu_reg_;
  std::array<DataT, 0x100> &ppu_oam_;
  ctrl::JoyPad &joypad_;
};

class PPUMap : public mem::Mapper {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8 - 2);

  explicit PPUMap(cart::Cartridge &c) : cart_(c) {}

  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  cart::Cartridge &cart_;
  std::array<DataT, 0x800> nametable_{};
  std::array<DataT, 32> palette_;
  AddressT mirror_vram_addr(AddressT addr);
};

} // namespace mapper
