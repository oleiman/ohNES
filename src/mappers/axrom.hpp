#pragma once

#include "apu_registers.hpp"
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
class AxROM : public NESMapperBase<AxROM> {
public:
  AxROM(sys::NES &console, cart::Cartridge const &c, vid::Registers &reg,
        aud::Registers &areg, std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<AxROM>(console, c, reg, areg, oam, pad) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);

  uint8_t mirroring() const override { return mirror | 0b10; }

private:
  uint8_t mirror = 0;
};

} // namespace mapper
