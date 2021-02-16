#include "ppu_registers.hpp"

#include <iostream>

using std::array;
using std::to_string;

namespace vid {
void Registers::write(CName r, uint8_t val) {
  if (!Writeable[r]) {
    // throw std::runtime_error("Attempt to write read-only PPU Register " +
    //                          to_string(r));
    std::cerr << "WARNING: Attempt to write to read-only PPU Register " +
                     to_string(r)
              << std::endl;
    return;
  }

  write_pending_ = false;
  // store lower 5 bits of data  in the status register
  regs_[PPUSTATUS] &= (0b11100000 | (val & 0b00011111));

  if (r == PPUSCROLL) {
    if (!addr_latch_) {
      scroll_addr_ = 0x0000;
      scroll_addr_ |= val;
      scroll_addr_ <<= 8;
    } else {
      scroll_addr_ |= val;
    }
    addr_latch_ = !addr_latch_;
  } else if (r == PPUADDR) {
    if (!addr_latch_) {
      vram_addr_ = 0x0000;
      vram_addr_ |= val;
      vram_addr_ <<= 8;
    } else {
      vram_addr_ |= val;
    }
    addr_latch_ = !addr_latch_;
  } else if (r == PPUDATA) {
    write_pending_ = true;
  }

  // TODO(oren): confusing control flow
  bool gen_nmi = vBlankNMI();
  regs_[r] = val;

  if (!gen_nmi && vBlankNMI() && vBlankStarted()) {
    nmi_pending_ = true;
  }

} // namespace ppu

uint8_t Registers::read(CName r) {
  if (!Readable[r]) {
    throw std::runtime_error("Attempt to read from write-only PPU Register " +
                             to_string(r));
  }
  read_pending_ = false;

  auto result = regs_[r];
  // TODO(oren): Refer to wiki for timing concerns, specifically around reading
  // from palette ram
  if (r == PPUSTATUS) {
    regs_[r] &= ~BIT7;
    addr_latch_ = false;
  } else if (r == PPUDATA) {

    // CPU reads from PPUDATA
    // setting the flag in the registers
    // in that same cycle, the PPU sees the pending read flag
    // reads from the stored vram address
    // calls putData with the result, which increments the address
    // and clears the pending read flag
    // now the right data is in PPUDATA for the next CPU read
    // which sets the flag again, triggering the PPU to read from
    // the incremented address
    read_pending_ = true;

    // TODO(oren): something special for palette (i.e. vram_add 0x3F00 -
    // 0x3FFF) need to be able to place a byte directly into PPUDATA and
    // return it
    result = regs_[PPUDATA];
  }
  return result;
}
uint8_t Registers::getData() {
  write_pending_ = false;
  vram_addr_ += vRamAddrInc();
  return regs_[PPUDATA];
}

void Registers::putData(uint8_t data) {
  read_pending_ = false;
  vram_addr_ += vRamAddrInc();
  regs_[PPUDATA] = data;
}

bool Registers::handleNmi() {
  auto tmp = nmi_pending_;
  nmi_pending_ = false;
  return tmp;
}

uint16_t Registers::oamCycles() {
  auto v = oam_cycles_;
  oam_cycles_ = 0;
  return v;
}

const array<bool, 8> Registers::Writeable = {true, true, false, true,
                                             true, true, true,  true};
const array<bool, 8> Registers::Readable = {false, false, true,  false,
                                            true,  false, false, true};

const array<uint16_t, 4> Registers::BaseNTAddr = {0x2000, 0x2400, 0x2800,
                                                  0x2C00};
const array<uint8_t, 2> Registers::VRamAddrInc = {0x01, 0x20};
const array<uint16_t, 2> Registers::PTableAddr = {0x000, 0x1000};
const std::array<std::array<uint8_t, 2>, 2> Registers::SpriteSize = {
    {{8, 8}, {8, 16}}};

} // namespace vid
