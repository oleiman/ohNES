#include "ppu.hpp"

#define BIT0 0b00000001
#define BIT1 0b00000010
#define BIT2 0b00000100
#define BIT3 0b00001000
#define BIT4 0b00010000
#define BIT5 0b00100000
#define BIT6 0b01000000
#define BIT7 0b10000000

using std::array;
using std::to_string;

namespace ppu {

void PPU::step(uint8_t cycles) {
  cycle_ += cycles;

  if (reg_.handleNmi()) {
    nmi_();
  }

  if (cycle_ >= 341) {
    cycle_ -= 341;
    ++scanline_;
    // std::cerr << "scanline: " << +scanline_ << std::endl;
    if (scanline_ == 241) {
      if (reg_.vBlankNMI()) {
        std::cerr << "generate nmi" << std::endl;
        nmi_();
      }
      std::cerr << "setting vblank" << std::endl;
      reg_.setVBlankStarted();

    } else if (scanline_ >= 262) {
      scanline_ = 0;
      reg_.clearVBlankStarted();
    }
  }
}

void PPU::tick() {}

const array<array<uint8_t, 3>, 64> PPU::SystemPalette = {
    {{0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0},
     {0x44, 0x00, 0x96}, {0xA1, 0x00, 0x5E}, {0xC7, 0x00, 0x28},
     {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00}, {0x5C, 0x2F, 0x00},
     {0x10, 0x45, 0x00}, {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E},
     {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05},
     {0x05, 0x05, 0x05}, {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF},
     {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA}, {0xEB, 0x2F, 0xB5},
     {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00},
     {0xC4, 0x62, 0x00}, {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00},
     {0x00, 0x8A, 0x55}, {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21},
     {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09}, {0xFF, 0xFF, 0xFF},
     {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF}, {0xD4, 0x80, 0xFF},
     {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33},
     {0xFF, 0x9C, 0x12}, {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E},
     {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4}, {0x05, 0xFB, 0xFF},
     {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D},
     {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF}, {0xB3, 0xEC, 0xFF},
     {0xDA, 0xAB, 0xEB}, {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3},
     {0xFF, 0xD2, 0xB0}, {0xFF, 0xEF, 0xA6}, {0xFF, 0xF7, 0x9C},
     {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
     {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11},
     {0x11, 0x11, 0x11}}};

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

    // TODO(oren): something special for palette (i.e. vram_add 0x3F00 - 0x3FFF)
    // need to be able to place a byte directly into PPUDATA and return it
  }
  return result;
}

inline uint16_t Registers::baseNametableAddr() {
  return BaseNTAddr[regs_[PPUCTRL] & (BIT0 | BIT1)];
}

inline uint8_t Registers::vRamAddrInc() {
  uint8_t i = (regs_[PPUCTRL] & BIT2) >> 2;
  return VRamAddrInc[i];
}

inline uint16_t Registers::spritePTableAddr() {
  uint8_t i = (regs_[PPUCTRL] & BIT3) >> 3;
  return PTableAddr[i];
}

inline uint16_t Registers::backgroundPTableAddr() {
  uint8_t i = (regs_[PPUCTRL] & BIT4) >> 4;
  return PTableAddr[i];
}

inline std::array<uint8_t, 2> const &Registers::spriteSize() {
  uint8_t i = (regs_[PPUCTRL] & BIT5) >> 5;
  return SpriteSize[i];
}

inline bool Registers::ppuMasterSlave() { return regs_[PPUCTRL] & BIT6; }

inline bool Registers::vBlankNMI() { return regs_[PPUCTRL] & BIT7; }

inline bool Registers::grayscale() { return regs_[PPUMASK] & BIT0; }
inline bool Registers::showBackgroundLeft8() { return regs_[PPUMASK] & BIT1; }
inline bool Registers::showSpritesLeft8() { return regs_[PPUMASK] & BIT2; }
inline bool Registers::showBackground() { return regs_[PPUMASK] & BIT3; }
inline bool Registers::showSprites() { return regs_[PPUMASK] & BIT4; }
inline bool Registers::emphasizeRed() { return regs_[PPUMASK] & BIT5; }
inline bool Registers::emphasizeGreen() { return regs_[PPUMASK] & BIT6; }
inline bool Registers::emphasizeBlue() { return regs_[PPUMASK] & BIT7; }

inline bool Registers::spriteOverflow() { return regs_[PPUSTATUS] & BIT5; }
inline bool Registers::spriteZeroHit() { return regs_[PPUSTATUS] & BIT6; }
inline bool Registers::vBlankStarted() { return regs_[PPUSTATUS] & BIT7; }

inline void Registers::setVBlankStarted() { regs_[PPUSTATUS] |= BIT7; }
inline void Registers::clearVBlankStarted() { regs_[PPUSTATUS] &= ~BIT7; }

inline uint8_t Registers::oamAddr() { return regs_[OAMADDR]; }
inline uint8_t Registers::oamData() { return regs_[OAMDATA]; }

// TODO(oren): logic for reading between the two prescribed register writes?
// again, this should probably go inside the PPU
inline uint16_t Registers::scrollAddr() { return scroll_addr_; }
inline uint16_t Registers::vRamAddr() { return vram_addr_; }

inline uint8_t Registers::getData() {
  vram_addr_ += vRamAddrInc();
  return regs_[PPUDATA];
}

inline void Registers::putData(uint8_t data) {
  vram_addr_ += vRamAddrInc();
  regs_[PPUDATA] = data;
}
inline bool Registers::writePending() { return write_pending_; }
inline bool Registers::readPending() { return read_pending_; }
inline bool Registers::handleNmi() {
  auto tmp = nmi_pending_;
  nmi_pending_ = false;
  return tmp;
}

const array<bool, 8> Registers::Writeable = {true, true, false, true,
                                             true, true, true,  true};
const array<bool, 8> Registers::Readable = {false, false, true,  false,
                                            true,  false, false, true};

const array<uint16_t, 4> Registers::BaseNTAddr = {0x2000, 0x2400, 0x2800,
                                                  0x2C00};
const array<uint8_t, 2> Registers::VRamAddrInc = {0x00, 0x20};
const array<uint16_t, 2> Registers::PTableAddr = {0x000, 0x1000};
const std::array<std::array<uint8_t, 2>, 2> Registers::SpriteSize = {
    {{8, 8}, {8, 16}}};

} // namespace ppu
