#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace mapper {
class MMC1 : public NESMapperBase<MMC1> {
  enum PrgRomBankMode { SWITCH32 = 0, FIXFIRST, FIXLAST };
  enum MirrorMode { ONELOWER = 0, ONEUPPER, VERTICAL, HORIZONTAL };
  enum ChrRomBankMode { SWITCH8 = 0, SWITCH2x4 };
  const static std::array<MirrorMode, 4> MirrorMap;
  const static std::array<PrgRomBankMode, 4> PrgMap;
  const static std::array<ChrRomBankMode, 2> ChrMap;

public:
  explicit MMC1(cart::Cartridge const &c, vid::Registers &reg,
                std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<MMC1>(c, reg, oam, pad) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);

private:
  // TODO(oren): naming
  uint8_t chrBankSelect2_ = 0;
  uint8_t shiftRegister_ = 0b00010000;
  bool prgRamEnable_ = true;
  PrgRomBankMode prgMode_ = FIXLAST;
  // NOTE(oren): chosen arbitrarily...
  MirrorMode mirrorMode_ = HORIZONTAL;
  ChrRomBankMode chrMode_ = SWITCH8;
  AddressT mirror_vram_addr(AddressT addr, uint8_t mirroring);
};
} // namespace mapper
