#include "mappers/mmc2.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

constexpr uint32_t PRG_BANK_SIZE = 0x2000;
constexpr uint32_t PRG_ADDR_MASK = PRG_BANK_SIZE - 1;
constexpr uint32_t CHR_BANK_SIZE = 0x1000;
constexpr uint32_t CHR_ADDR_MASK = CHR_BANK_SIZE - 1;

namespace mapper {

void MMC2::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x6000) {
    return;
  } else if (addr < 0x8000) {
    assert(false);
    cart_.prgRam[addr & PRG_ADDR_MASK] = data;
  } else if (addr < 0xA000) {
    return;
  } else if (addr < 0xB000) {
    prgBankSelect = data & 0b1111;
  } else if (addr < 0xC000) {
    chrBankSelect[0] = data & 0b11111;
  } else if (addr < 0xD000) {
    chrBankSelect[1] = data & 0b11111;
  } else if (addr < 0xE000) {
    chrBankSelect[2] = data & 0b11111;
  } else if (addr < 0xF000) {
    chrBankSelect[3] = data & 0b11111;
  } else {
    mirroring_ = data & 0b1;
  }
}

MMC2::DataT MMC2::cartRead(AddressT addr) {
  uint32_t bank = 0;
  AddressT offset = addr & PRG_ADDR_MASK;
  if (addr < 0x6000) {
    return 0;
  } else if (addr < 0x8000) {
    assert(false);
    // no prg ram here
    return cart_.prgRam[addr];
  } else if (addr < 0xA000) {
    bank = PRG_BANK_SIZE * prgBankSelect;
  } else if (addr < 0xC000) {
    bank = cart_.prgRomSize - (3 * PRG_BANK_SIZE);
  } else if (addr < 0xE000) {
    bank = cart_.prgRomSize - (2 * PRG_BANK_SIZE);
  } else {
    bank = cart_.prgRomSize - PRG_BANK_SIZE;
  }
  return cart_.prgRom[bank + offset];
}
void MMC2::chrWrite(AddressT addr, DataT data) {
  assert(false);
  // no chr ram
}

MMC2::DataT MMC2::chrRead(AddressT addr) {
  AddressT offset = addr & CHR_ADDR_MASK;
  uint32_t bank = 0;
  if (addr < 0x1000) {
    auto l = latch[0];
    if (l == 0xFD) {
      bank = CHR_BANK_SIZE * chrBankSelect[0];
    } else if (l == 0xFE) {
      bank = CHR_BANK_SIZE * chrBankSelect[1];
    } else {
      assert(false);
    }
  } else {
    auto l = latch[1];
    if (l == 0xFD) {
      bank = CHR_BANK_SIZE * chrBankSelect[2];
    } else if (l == 0xFE) {
      bank = CHR_BANK_SIZE * chrBankSelect[3];
    } else {
      assert(false);
    }
  }

  if (addr == 0x0FD8) {
    latch[0] = 0xFD;
  } else if (addr == 0x0FE8) {
    latch[0] = 0xFE;
  } else if (0x1FD8 <= addr && addr <= 0x1FDF) {
    latch[1] = 0xFD;
  } else if (0x1FE8 <= addr && addr <= 0x1FEF) {
    latch[1] = 0xFE;
  }

  return cart_.chrRom[bank + offset];
}
} // namespace mapper
