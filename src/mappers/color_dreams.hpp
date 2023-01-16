#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace sys {
class NES;
}

namespace mapper {

class ColorDreams : public NESMapperBase<ColorDreams> {
public:
  ColorDreams(sys::NES &console, cart::Cartridge const &c, vid::Registers &reg,
              std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<ColorDreams>(console, c, reg, oam, pad) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);
};

} // namespace mapper
