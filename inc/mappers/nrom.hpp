#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {
class NROM : public BaseNESMapper {
public:
  NROM(cart::Cartridge const &c, vid::Registers &reg,
       std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : BaseNESMapper(c, reg, oam, pad) {}

  // TODO(oren): don't think this is needed
  constexpr static size_t size = 1ul << (sizeof(AddressT) * 8);
  void write(AddressT addr, DataT data) override;
  DataT read(AddressT addr) override;
};
} // namespace mapper
