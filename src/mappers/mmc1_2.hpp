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
class MMC1 : public NESMapperBase<MMC1> {

public:
  explicit MMC1(sys::NES &console, cart::Cartridge const &c,
                vid::Registers &reg, aud::Registers &areg,
                std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<MMC1>(console, c, reg, areg, oam, pad) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);

  uint8_t mirroring() const override {
    auto m = control_ & 0b11;
    uint8_t result = 0;
    switch (m) {
    case 0:
      result = 2;
      break;
    case 1:
      result = 3;
      break;
    case 2:
      result = 1;
      break;
    case 3:
      result = 0;
      break;
    }
    return result;
  }

private:
  static constexpr uint8_t sr_init_ = 0b1 << 4;
  void writeInternal(AddressT addr, uint8_t data);
  void clearSR() { shiftReg_ = sr_init_; }
  void reset() {
    clearSR();
    // set PRG to mode 3
    control_ |= 0x0C;
    // control_ = 0x00;
  }

  uint8_t prgMode() { return (control_ >> 2) & 0b11; }
  uint8_t chrMode() { return (control_ >> 4) & 0b1; }
  bool prgRamEnabled() { return !(prgBankSelect_ & 0b10000); }

  // TODO(oren): this might not be the right initial val for control reg
  uint8_t control_ = 0x0C;
  uint8_t shiftReg_ = sr_init_;
  std::array<uint8_t, 2> chrBankSelect_ = {};
  uint8_t prgBankSelect_ = 0x00;
  unsigned long long last_sr_load_ = 0;
};
} // namespace mapper
