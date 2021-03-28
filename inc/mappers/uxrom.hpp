#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {
class UxROM : public NESMapperBase<UxROM> {
public:
  explicit UxROM(cart::Cartridge const &c, vid::Registers &reg,
                 std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<UxROM>(c, reg, oam, pad) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);
};
} // namespace mapper
