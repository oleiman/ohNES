#include "mappers/mmc3.hpp"
#include "system.hpp"

#include <iostream>
#include <sstream>

using CName = vid::Registers::CName;

namespace mapper {

void MMC3::cartWrite(AddressT addr, DataT data) {
  bool even = addr % 2 == 0;
  if (addr < 0x8000) {
    if (prgRamEnabled() && !prgRamWriteProtect()) {
      cart_.prgRam[addr & (cart_.prgRamSize - 1)] = data;
    }
  } else if (addr < 0xA000) {
    if (even) {
      prgBankSelect = data;
    } else {
      auto sel = prgBankSelect & 0b111;
      if (sel == 6 || sel == 7) {
        // PRG ignores the upper 2 bits
        data &= 0b00111111;
      } else if (sel == 0 || sel == 1) {
        // 2kb banks ignore the bottom bit (still select in 1KiB units)
        data &= 0b11111110;
      }
      bankConfig_[sel] = data;
    }
  } else if (addr < 0xC000) {
    if (even) {
      mirroring_ = data & 0b1;
    } else {
      prgRamProtect_ = data;
    }
  } else if (addr < 0xE000) {
    if (even) {
      irqLatchVal_ = data;
    } else {
      irqCounter_.clear();
    }
  } else { // <= 0xFFFF
    // even reg disables, odd reg enables
    irqEnable(!even);
  }
}

// 0x2000 is the prg bank size
MMC3::DataT MMC3::cartRead(AddressT addr) {
  if (addr < 0x8000) {
    if (prgRamEnabled()) {
      return cart_.prgRam[addr & (cart_.prgRamSize - 1)];
    } else {
      return 0;
    }
  }
  const AddressT bank_size = 0x2000;
  AddressT offset = addr & (bank_size - 1);
  uint32_t bank = 0;
  if (addr < 0xA000) {
    bank = (prgMapMode() ? (cart_.prgRomSize - (bank_size << 1))
                         : (bankConfig_[6] * bank_size));
  } else if (addr < 0xC000) {
    bank = bankConfig_[7] * bank_size;
  } else if (addr < 0xE000) {
    bank = (prgMapMode() ? (bankConfig_[6] * bank_size)
                         : (cart_.prgRomSize - (bank_size << 1)));
  } else {
    bank = cart_.prgRomSize - bank_size;
  }
  return cart_.prgRom[bank + offset];
}

void MMC3::chrWrite(AddressT addr, DataT data) {
  assert(cart_.chrRamSize);
  const AddressT bank_size = 0x400;
  AddressT offset = addr & (bank_size - 1);
  uint32_t bank = 0;
  if (chrMapMode()) {
    if (addr < 0x400) {
      bank = bankConfig_[2] * bank_size;
    } else if (addr < 0x800) {
      bank = bankConfig_[3] * bank_size;
    } else if (addr < 0xC00) {
      bank = bankConfig_[4] * bank_size;
    } else if (addr < 0x1000) {
      bank = bankConfig_[5] * bank_size;
    } else if (addr < 0x1800) {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[0] * bank_size;
    } else {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[1] * bank_size;
    }
  } else {
    if (addr < 0x800) {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[0] * bank_size;
    } else if (addr < 0x1000) {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[1] * bank_size;
    } else if (addr < 0x1400) {
      bank = bankConfig_[2] * bank_size;
    } else if (addr < 0x1800) {
      bank = bankConfig_[3] * bank_size;
    } else if (addr < 0x1C00) {
      bank = bankConfig_[4] * bank_size;
    } else {
      bank = bankConfig_[5] * bank_size;
    }
  }
  // setPpuABus(bank + offset);
  cart_.chrRam[bank + offset] = data;
}

// 0x400 is the chr bank size for the purposes of indexing (incl for 2kb banks)
MMC3::DataT MMC3::chrRead(AddressT addr) {
  const AddressT bank_size = 0x400;
  AddressT offset = addr & (bank_size - 1);
  uint32_t bank = 0;
  if (chrMapMode()) {
    if (addr < 0x400) {
      bank = bankConfig_[2] * bank_size;
    } else if (addr < 0x800) {
      bank = bankConfig_[3] * bank_size;
    } else if (addr < 0xC00) {
      bank = bankConfig_[4] * bank_size;
    } else if (addr < 0x1000) {
      bank = bankConfig_[5] * bank_size;
    } else if (addr < 0x1800) {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[0] * bank_size;
    } else {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[1] * bank_size;
    }
  } else {
    if (addr < 0x800) {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[0] * bank_size;
    } else if (addr < 0x1000) {
      offset = addr & ((bank_size << 1) - 1);
      bank = bankConfig_[1] * bank_size;
    } else if (addr < 0x1400) {
      bank = bankConfig_[2] * bank_size;
    } else if (addr < 0x1800) {
      bank = bankConfig_[3] * bank_size;
    } else if (addr < 0x1C00) {
      bank = bankConfig_[4] * bank_size;
    } else {
      bank = bankConfig_[5] * bank_size;
    }
  }
  // setPpuABus(bank + offset);
  if (cart_.chrRamSize) {
    return cart_.chrRam[bank + offset];
  } else {
    return cart_.chrRom[bank + offset];
  }
}

void MMC3::irqEnable(bool e) {
  // if interrupts are on and we're turning them off,
  // acknowledge whatever is pending
  if (irqEnabled_ && !e) {
    // TODO(oren): acknowledge pending interrupts
    pending_irq_ = false;
    console_.irqPin() = false;
  }
  irqEnabled_ = e;
}

bool MMC3::genIrq() {
  if (!pending_irq_) {
    console_.irqPin() = true;
    pending_irq_ = true;
    return true;
  }
  return false;
}

// HACK(oren): LOL
bool MMC3::setPpuABus(AddressT val) {
  bool prev = (ppuABus_ & 0x1000);
  bool next = (val & 0x1000);
  ppuABus_ = val;

  // only do the thing on a falling edge on bit 12 (0 -> 1)
  // unsigned long long time_since_change = 0;
  // if (prev != next) {
  //   if (a12_state_.timer > 0)
  //     time_since_change = m2_count_ - a12_state_.timer;
  //   a12_state_.state = (next ? A12_STATE::HIGH : A12_STATE::LOW);
  //   a12_state_.timer = m2_count_;
  // }

  if (!prev && next // && time_since_change >= 1
  ) {
    if (irqCounter_.reload(irqLatchVal_)) {
      return false;
    }

    irqCounter_.val--;
    if (irqCounter_.val == 0 && irqEnabled_) {
      return genIrq();
    }
  }
  return false;
}

} // namespace mapper
