#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {

class PPUMap : public mem::Mapper {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8 - 2);

  explicit PPUMap(cart::Cartridge const &c) : cart_(c) {}

  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  cart::Cartridge const &cart_;
  std::array<DataT, 0x800> nametable_{};
  std::array<DataT, 32> palette_;
  AddressT mirror_vram_addr(AddressT addr);
};

} // namespace mapper
