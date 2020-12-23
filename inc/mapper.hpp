#pragma once

#include "cartridge.hpp"

#include <array>

namespace mapper {
class NROM {
public:
  using AddressT = uint16_t;
  using DataT = uint8_t;

  explicit NROM(cart::Cartridge &c, std::array<DataT, 0x800> &cpu,
                std::array<DataT, 8> &ppu_reg)
      : cart_(c), internal_(cpu), ppu_reg_(ppu_reg) {}
  ~NROM() = default;

  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8);
  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  // 2KB of internal RAM
  cart::Cartridge &cart_;
  std::array<DataT, 0x800> &internal_;
  std::array<DataT, 8> &ppu_reg_;
};

class PPUMap {
  using AddressT = uint16_t;
  using DataT = uint8_t;

  explicit PPUMap(cart::Cartridge &c, std::array<DataT, 0x800> &nt)
      : nametable_(nt) {}

  void write(AddressT addr, DataT data);
  DataT read(AddressT addr);

private:
  std::array<DataT, 8> registers_;
  std::array<DataT, 0x800> &nametable_;
};

} // namespace mapper
