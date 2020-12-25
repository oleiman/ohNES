#pragma once

#include "cartridge.hpp"
#include "ppu.hpp"

#include <array>

namespace mapper {
class NROM {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;

  explicit NROM(cart::Cartridge &c, std::array<DataT, 0x800> &cpu,
                ppu::Registers &reg)
      : cart_(c), internal_(cpu), ppu_reg_(reg) {}
  ~NROM() = default;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8);
  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  // 2KB of internal RAM
  cart::Cartridge &cart_;
  std::array<DataT, 0x800> &internal_;
  ppu::Registers &ppu_reg_;
};

class PPUMap {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8 - 2);

  explicit PPUMap(cart::Cartridge &c, std::array<DataT, 0x800> &nt)
      : cart_(c), nametable_(nt) {}

  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  cart::Cartridge &cart_;
  std::array<DataT, 0x800> &nametable_;
  std::array<DataT, 32> palette_;
  AddressT mirror_vram_addr(AddressT addr);
};

} // namespace mapper
