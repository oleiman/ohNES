#include "mappers/mmc1_2.hpp"
#include "system.hpp"

using CName = vid::Registers::CName;

constexpr uint32_t PRG_RAM_SIZE = 0x2000;
constexpr uint32_t PRG_RAM_MASK = PRG_RAM_SIZE - 1;
constexpr uint32_t PRG_BANK_SIZE = 0x4000;
constexpr uint32_t PRG_ADDR_MASK = PRG_BANK_SIZE - 1;
constexpr uint32_t CHR_BANK_SIZE = 0x1000;
constexpr uint32_t CHR_ADDR_MASK = CHR_BANK_SIZE - 1;

namespace mapper {

void MMC1::cartWrite(AddressT addr, DataT data) {
  if (addr < 0x8000) {
    if (prgRamEnabled()) {
      assert((addr & PRG_RAM_MASK) < cart_.prgRamSize);
      cart_.prgRam[addr & PRG_RAM_MASK] = data;
    }
  } else if (data & 0b10000000) {
    reset();
  } else if (last_sr_load_ == 0 ||
             (console_.state().cycle - last_sr_load_) > 1) {
    bool full = shiftReg_ & 0b1;
    shiftReg_ >>= 1;
    shiftReg_ |= ((data & 0b1) << 4);
    if (full) {
      writeInternal(addr, shiftReg_);
      clearSR();
    }
  }
  last_sr_load_ = console_.state().cycle;
}

void MMC1::writeInternal(AddressT addr, uint8_t data) {

  assert(addr >= 0x8000);

  if (addr < 0xA000) {
    control_ = data;
  } else if (addr < 0xC000) {
    chrBankSelect_[0] = data;
  } else if (addr < 0xE000) {
    chrBankSelect_[1] = data;
  } else {
    prgBankSelect_ = data;
  }
}

MMC1::DataT MMC1::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    // return cart_.prgRam[addr & PRG_RAM_MASK];
    if (prgRamEnabled()) {
      return cart_.prgRam[addr & PRG_RAM_MASK];
    } else {
      return 0;
    }
  }

  uint32_t bank = 0;
  AddressT offset = addr & PRG_ADDR_MASK;

  switch (prgMode()) {
  case 0:
  case 1:
    // TODO(oren): don't think we need to double the bank size here
    bank = PRG_BANK_SIZE * (prgBankSelect_ & 0b1110);
    // but we do need to double the mask
    offset = addr & ((PRG_BANK_SIZE * 2) - 1);
    break;
  case 2:
    if (addr < 0xC000) {
      // fixed to first bank
      bank = 0;
    } else {
      bank = PRG_BANK_SIZE * (prgBankSelect_ & 0b1111);
    }
    break;
  case 3:
    if (addr < 0xC000) {
      bank = PRG_BANK_SIZE * (prgBankSelect_ & 0b1111);
    } else {
      // fixed to last bank
      bank = cart_.prgRomSize - PRG_BANK_SIZE;
    }
    break;
  default:
    assert(false);
  }

  assert(bank + offset < cart_.prgRomSize);
  return cart_.prgRom[bank + offset];
}

void MMC1::chrWrite(AddressT addr, DataT data) {
  if (cart_.chrRamSize) {
    uint32_t bank = 0;
    AddressT offset = addr & CHR_ADDR_MASK;
    if (chrMode() == 0) {
      // 8K switched
      bank = 0;
      offset = addr;
    } else if (addr < 0x1000) {
      bank = CHR_BANK_SIZE * (chrBankSelect_[0] & 0b1);
    } else {
      bank = CHR_BANK_SIZE * (chrBankSelect_[1] & 0b1);
    }
    assert(bank + offset < cart_.chrRamSize);
    cart_.chrRam[bank + offset] = data;
  } else {
    assert(false);
  }
}

MMC1::DataT MMC1::chrRead(AddressT addr) {
  uint32_t bank = 0;
  AddressT offset = addr & CHR_ADDR_MASK;

  if (cart_.chrRamSize) {
    if (chrMode() == 0) {
      bank = 0;
      offset = addr;
    } else if (addr < 0x1000) {
      bank = CHR_BANK_SIZE * (chrBankSelect_[0] & 0b1);
    } else {
      bank = CHR_BANK_SIZE * (chrBankSelect_[1] & 0b1);
    }
    assert(bank + offset < cart_.chrRamSize);
    return cart_.chrRam[bank + offset];
  } else {
    if (chrMode() == 0) {
      // 8K switched
      bank = CHR_BANK_SIZE * (chrBankSelect_[0] & 0b11110);
      // offset = addr & ((CHR_BANK_SIZE << 1) - 1);
      offset = addr;
    } else if (addr < 0x1000) {
      bank = CHR_BANK_SIZE * (chrBankSelect_[0] & 0b11111);
    } else {
      bank = CHR_BANK_SIZE * (chrBankSelect_[1] & 0b11111);
    }
    return cart_.chrRom[bank + offset];
  }
}

} // namespace mapper
