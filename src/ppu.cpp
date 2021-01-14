#include "ppu.hpp"

#include <algorithm>
#include <bitset>

using std::array;
using std::to_string;

namespace vid {

void PPU::dumbStep(uint16_t cycles) {
  if (reg_.handleNmi()) {
    std::cout << "out of band nmi" << std::endl;
    nmi_();
  }
  while (--cycles > 0) {
    tick();
    if (scanline_ == 241) {
      reg_.setVBlankStarted();
    }
  }
}
void PPU::step(uint16_t cycles) {

  if (reg_.handleNmi()) {
    std::cout << "out of band nmi" << std::endl;
    nmi_();
  }

  // TODO(oren): this leads to a BRK at some point and we get burned
  // trying to write to cartridge ROM
  cycles += reg_.oamCycles();

  while (cycles-- > 0) {
    if (scanline_ >= 262) {
      reg_.clearVBlankStarted();
      scanline_ = 0;
    }

    if (!(reg_.showBackground() || reg_.showSprites())) {
      vBlankLine();
    } else if (scanline_ < 240) {
      visibleLine();
    } else if (scanline_ == 240) {
      tick();
    } else if (scanline_ < 261) { // VBlank lines
      vBlankLine();
    } else if (scanline_ == 261) {
      // TODO(oren): pre-render scanline
      if (cycle_ == 1) {
      }
      tick();
    }
  }
}

void PPU::visibleLine() {
  // std::cerr << " render a visible line" << std::endl;
  if (cycle_ == 0) {
    tick();
    return;
  }

  if (reg_.writePending() || reg_.readPending()) {
    if (reg_.writePending()) {
      throw std::runtime_error("CPU writing during non-VBLANK line");
    } else {
      throw std::runtime_error("CPU reading during non-VBLANK line");
    }
  }

  bool odd = cycle_ & 0x01;

  if (cycle_ < 257) { // Data for current scanline
    if (((cycle_ - 1) & 0x07) == 0) {
      // TODO(oren): reload shift registers
    }
    if (odd) {
      // TODO(oren): READ into internal latches
      // 1. Nametable byte
      // 2. Attribute table byte
      // 3. Pattern table tile low
      // 4. Pattern table tile high (+8 bytes from pattern table tile low)
    } else {
      // "finalize" the memory access from the previous cycle)
    }
    // bookkeeping
  } else if (cycle_ < 321) {
    oam_2_.fill(0);
    // TODO(oren): sprite evaluation
  } else if (cycle_ < 336) {
    // TODO(oren): load first 2 tiles for next scanline into shift registers
  } else {
    // TODO(oren): throwaway reads (cycles 337 - 340)
  }
  tick();
}

void PPU::vBlankLine() {
  if (cycle_ == 1 && scanline_ == 241) {
    if (reg_.vBlankNMI()) {
      nmi_();
    }
    reg_.setVBlankStarted();
  }

  bool odd = cycle_ & 0x01;

  // TODO(oren): service CPU memory requests
  // again only on odd cycles to account for 2-cycle cost. may need to look into
  // this a bit more
  if (odd) {
    if (reg_.readPending()) {
      // std::cout << "service read" << std::endl;
      auto addr = reg_.vRamAddr();
      reg_.putData(readByte(addr));
    } else if (reg_.writePending()) {
      auto addr = reg_.vRamAddr();
      auto data = reg_.getData();
      writeByte(addr, data);
    }
  }
  tick();
}

std::array<uint8_t, 4> PPU::bgPalette(uint16_t base, uint16_t tile_x,
                                      uint16_t tile_y) {
  uint16_t at_base = base + 0x3c0;
  uint8_t block_x = tile_x >> 2;
  uint8_t block_y = tile_y >> 2;
  auto at_entry = readByte(at_base + (block_y * 8 + block_x));
  uint8_t meta_x = (tile_x & 0x03) >> 1;
  uint8_t meta_y = (tile_y & 0x03) >> 1;
  uint8_t shift = (meta_y * 2 + meta_x) << 1;
  uint8_t pidx = ((at_entry >> shift) & 0b11) << 2;
  return {readByte(0x3f00), readByte(0x3f01 + pidx), readByte(0x3f02 + pidx),
          readByte(0x3f03 + pidx)};
}

std::array<uint8_t, 4> PPU::spritePalette(uint8_t pidx) {
  pidx <<= 2;
  return {0, readByte(0x3f01 + pidx), readByte(0x3f02 + pidx),
          readByte(0x3f03 + pidx)};
}

inline void PPU::tick() {
  ++cycle_;
  if (cycle_ == 341) {
    ++scanline_;
    cycle_ = 0;
  }
}

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

} // namespace vid
