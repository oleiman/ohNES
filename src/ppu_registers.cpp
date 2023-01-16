#include "ppu_registers.hpp"

#include <iostream>

using std::array;
using std::to_string;

namespace vid {
void Registers::write(CName r, uint8_t val) {
  // store lower 5 bits of data  in the status register
  regs_[PPUSTATUS] &= (0b11100000 | (val & 0b00011111));
  // also store the write value to the i/o latch
  io_latch_ = val;

  if (!Writeable[r]) {
    std::cerr << "WARNING: Attempt to write to read-only PPU Register " +
                     to_string(r)
              << std::endl;
    return;
  }

  write_pending_ = false;

  if (r == PPUSCROLL) {
    if (!addr_latch_) {
      T.XXXXX = val >> 3;
      x = val & 0b111;
    } else {
      T.YYYYY = val >> 3;
      T.yyy = val & 0b111;
    }
    addr_latch_ = !addr_latch_;
  } else if (r == PPUADDR) {
    if (!addr_latch_) {
      vram_addr_ = 0x0000;
      // NOTE(oren): lowerpp 6 bits
      vram_addr_ |= (val & 0x3F);
      vram_addr_ <<= 8;
      T.yyy = (val >> 5) & 0b11;
      T.NN = (val >> 3) & 0b11;
      T.YYYYY &= 0b111;
      T.YYYYY |= (val & 0b11) << 3;
    } else {
      vram_addr_ |= val;
      T.YYYYY &= (0b11000);
      T.YYYYY |= (val >> 5) & 0b111;
      T.XXXXX = val & 0b11111;
      syncScroll();
    }
    addr_latch_ = !addr_latch_;
  } else if (r == PPUDATA) {
    write_pending_ = true;
  } else if (r == PPUCTRL) {
    T.NN = val & 0b11;
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
    return io_latch_;
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
  io_latch_ = result;
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
const std::array<uint8_t, 2> Registers::SpriteSize = {8, 16};

} // namespace vid
