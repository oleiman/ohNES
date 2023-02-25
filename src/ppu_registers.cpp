#include "ppu_registers.hpp"
#include "mappers/base_mapper.hpp"

#include <iostream>

using std::array;
using std::to_string;

namespace vid {
void Registers::write(CName r, uint8_t val, mapper::NESMapper &mapper) {
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
      mapper.setPpuABus(vram_addr_);
      T.yyy = (val >> 4) & 0b11;
      T.NN = (val >> 2) & 0b11;
      T.YYYYY &= 0b111;
      T.YYYYY |= (val & 0b11) << 3;
    } else {
      vram_addr_ |= val;
      mapper.setPpuABus(vram_addr_);
      T.YYYYY &= (0b11000);
      T.YYYYY |= (val >> 5) & 0b111;
      T.XXXXX = val & 0b11111;
      syncScroll();
    }
    addr_latch_ = !addr_latch_;
  } else if (r == PPUDATA) {
    // If we're writing to palette ram, perform the write right away,
    // increment the vram addr, and suppress the usual VRAM write
    if ((vram_addr_ & 0x3F00) == 0x3F00) {
      mapper.palette_write(vram_addr_, val);
      vram_addr_ += vRamAddrInc();
      write_pending_ = false;
    } else {
      write_pending_ = true;
      write_value_ = val;
    }
  } else if (r == PPUCTRL) {
    T.NN = val & 0b11;
  } else if (r == OAMDATA) {
    mapper.oam_write(oamAddr(), val);
    regs_[OAMADDR]++;
  }

  // VBL NMI setting before the write
  bool vblnmi_orig = vBlankNMI();

  // VRAM writes should not overwrite the read buffer
  if (r != PPUDATA) {
    regs_[r] = val;
  }

  // NOTE(oren): this only applies to PPUCTRL writes
  if (!vblnmi_orig && vBlankNMI() && vBlankStarted()) {
    // If we're in vblank and NMI is turned on, instruct PPU to generate an NMI
    // at next opportunity
    nmi_pending_ = true;
  } else if (vblnmi_orig && !vBlankNMI() && cycle_ <= 3) {
    // Otherwise if vblank was just turned off and NMI may be about to trigger,
    // instruct PPU to suppress that NMI signal
    suppress_vblank_ = true;
  }

} // namespace ppu

uint8_t Registers::read(CName r, mapper::NESMapper &mapper) {
  if (!Readable[r]) {
    return io_latch_;
  }

  read_pending_ = false;

  auto result = regs_[r];
  // TODO(oren): Refer to wiki for timing concerns, specifically around reading
  // from palette ram
  if (r == PPUSTATUS) {
    clearVBlankStarted();
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

    // TODO(oren): set the address bus to something I guess? but what?
    // blargg tests suggest that reading ppudata should
    // clock the irq counter but idk how or why
    // std::cout << std::hex << vram_addr_ << std::endl;
    read_pending_ = true;

    if ((vram_addr_ & 0x3F00) == 0x3F00) {
      result = mapper.palette_read(vram_addr_);
      if (grayscale()) {
        result &= 0x30;
      }

    } else {
      result = regs_[PPUDATA];
    }
  } else if (r == OAMDATA) {
    result = mapper.oam_read(oamAddr());
  }
  io_latch_ = result;
  return result;
}

void Registers::tick() {
  ++cycle_;
  if (cycle_ == 341) {
    ++scanline_;
    cycle_ = 0;
    if (scanline_ == 241) {
      frame_ready_ = true;
    } else if (scanline_ == 262) {
      nextFrame();
    }
  }
}

bool Registers::setVBlankStarted() {
  if (!suppress_vblank_) {
    regs_[PPUSTATUS] |= util::BIT7;
    return true;
  } else {
    return false;
  }
}

void Registers::nextFrame() {
  scanline_ = 0;
  suppress_vblank_ = false;
  ++frame_count_;
}

// TODO(oren): move this bit of state into the PPU proper I think.
bool Registers::isFrameReady() {
  auto tmp = frame_ready_;
  frame_ready_ = false;
  return tmp;
}

void Registers::clearVBlankStarted() {
  regs_[PPUSTATUS] &= ~util::BIT7;
  // clear any pending NMI if VBL ends
  nmi_pending_ = false;
  if (scanline_ == 241 && (0 < cycle_ && cycle_ <= 3)) {
    suppress_vblank_ = true;
  }
}

uint8_t Registers::getData() {
  write_pending_ = false;
  vram_addr_ += vRamAddrInc();
  return write_value_;
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
