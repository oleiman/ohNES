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

class MMC2 : public NESMapperBase<MMC2> {
public:
  MMC2(sys::NES &console, cart::Cartridge const &c, vid::Registers &reg,
       aud::Registers &areg, std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<MMC2>(console, c, reg, areg, oam, pad) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);

  uint8_t mirroring() const override {
    // NOTE(oren): for this mapper, 0 means vertical, 1 means horizontal
    // rest of emu expects the opposite, so we flip the mirroring bit
    return mirroring_ ^ 0b1;
  }

private:
  std::array<uint8_t, 4> chrBankSelect = {};
  std::array<uint8_t, 2> latch = {0xFD, 0xFD};
  uint8_t mirroring_ = 0;
};

} // namespace mapper
