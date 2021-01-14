#include "ppu_registers.hpp"

#include <iostream>

using std::array;
using std::to_string;

namespace vid {
void Registers::write(CName r, uint8_t val) {
  if (!Writeable[r]) {
    // TODO(oren): decide on some failure mode
    throw std::runtime_error("Attempt to write read-only PPU Register " +
                             to_string(r));
  }

  write_pending_ = false;
  // store lower 5 bits of the write in the status register
  regs_[PPUSTATUS] &= (0b11100000 | (val & 0b00011111));

  // TODO(oren): this should maybe go inside the PPU itself
  // need to figure out how to manage state between PPU cycles
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
    // std::cout << +val << std::endl;
    if (!addr_latch_) {
      vram_addr_ = 0x0000;
      vram_addr_ |= val;
      vram_addr_ <<= 8;
    } else {
      vram_addr_ |= val;
      // std::cout << "PPU VRAM Addr: " << std::hex << +vram_addr_ << std::dec
      //           << std::endl;
    }
    addr_latch_ = !addr_latch_;
  } else if (r == PPUDATA) {
    write_pending_ = true;
  } // else if (r == PPUMASK) {

  // }

  // TODO(oren): control flow...
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
  // TODO(oren): this might have timing implications
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

    // std::cerr << "read from PPUDATA: " << std::hex << +vram_addr_ << ": "
    //           << std::dec << +regs_[PPUDATA] << std::endl;

    // TODO(oren): something special for palette (i.e. vram_add 0x3F00 -
    // 0x3FFF) need to be able to place a byte directly into PPUDATA and
    // return it

    result = regs_[PPUDATA];
  }
  return result;
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

// #define BIT0 0b00000001
// #define BIT1 0b00000010
// #define BIT2 0b00000100
// #define BIT3 0b00001000
// #define BIT4 0b00010000
// #define BIT5 0b00100000
// #define BIT6 0b01000000
// #define BIT7 0b10000000

} // namespace vid
