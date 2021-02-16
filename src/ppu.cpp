#include "ppu.hpp"

#include <algorithm>
#include <bitset>

using std::array;
using std::to_string;

namespace vid {

void PPU::step(uint16_t cycles, bool &nmi) {

  if (registers.handleNmi()) {
    nmi = true;
  }

  cycles += registers.oamCycles();

  while (cycles-- > 0) {
    if (cycle_ == 1 && scanline_ == 241) {
      if (registers.vBlankNMI()) {
        nmi = true;
      }
      registers.setVBlankStarted();
      registers.setSpriteZeroHit(false);
    } else if (scanline_ >= 262) {
      registers.clearVBlankStarted();
      registers.setSpriteZeroHit(false);
      scanline_ = 0;
    }

    if (!(registers.showBackground() || registers.showSprites())) {
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
  if (cycle_ == 0) {
    tick();
    return;
  }

  if (registers.writePending() || registers.readPending()) {
    if (registers.writePending()) {
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
  bool odd = cycle_ & 0x01;

  // TODO(oren): service CPU memory requests
  // again only on odd cycles to account for 2-cycle cost. may need to look into
  // this a bit more
  if (odd) {
    if (registers.readPending()) {
      // std::cout << "service read" << std::endl;
      auto addr = registers.vRamAddr();
      registers.putData(readByte(addr));
    } else if (registers.writePending()) {
      auto addr = registers.vRamAddr();
      auto data = registers.getData();
      writeByte(addr, data);
    }
  }
  tick();
}

bool PPU::render() {
  auto bg = renderBackground();
  auto spr = renderSprites();
  return bg || spr;
}

bool PPU::renderBackground() {
  if (registers.showBackground()) {
    auto primary_nt = registers.baseNametableAddr();
    auto secondary_nt = 0x2000 + ((primary_nt - 0x2000 + 0x400) % 0x800);
    if (primary_nt > 0x2400) {
      std::cout << "holy cow" << std::endl;
    }

    // TODO(oren): not quite right for vertical scroll
    renderNametable(primary_nt,
                    {registers.scrollX(), registers.scrollY(), 255, 239},
                    -registers.scrollX(), -registers.scrollY());
    renderNametable(secondary_nt, {0, 0, registers.scrollX(), 239},
                    255 - registers.scrollX(), 0);
    return true;
  } else {
    return false;
  }
}

void PPU::renderNametable(uint16_t nt_base, Viewable const &view, int shift_x,
                          int shift_y) {
  auto bank = registers.backgroundPTableAddr();

  for (int i = 0; i < 0x03c0; ++i) {
    auto tile = static_cast<uint16_t>(readByte(nt_base + i));
    auto tile_x = i % 32;
    auto tile_y = i / 32;
    auto palette = bgPalette(nt_base, tile_x, tile_y);
    auto tile_base = bank + tile * 16;
    for (int y = 0; y < 8; ++y) {
      auto lower = readByte(tile_base + y);
      auto upper = readByte(tile_base + y + 8);
      for (int x = 7; x >= 0; x--) {
        auto value = ((upper & 1) << 1) | (lower & 1);
        upper >>= 1;
        lower >>= 1;
        uint8_t rgb = palette[value];
        uint8_t pixel_x = tile_x * 8 + x;
        uint8_t pixel_y = tile_y * 8 + y;

        if (pixel_x >= view.x_min && pixel_x <= view.x_max &&
            pixel_y >= view.y_min && pixel_y <= view.y_max) {
          assert((pixel_x + shift_x) < 256);
          assert((pixel_y + shift_y) < 240);
          set_pixel(pixel_x + shift_x, pixel_y + shift_y, SystemPalette[rgb]);
        }
      }
    }
  }
}

bool PPU::renderSprites() {
  if (registers.showSprites()) {
    for (int i = 0; i < oam.size(); i += 4) {
      // uint8_t sprite_idx = i >> 2;
      uint8_t tile_idx = oam[i + 1];
      uint8_t sprite_x = oam[i + 3];
      uint8_t sprite_y = oam[i];
      uint8_t attr = oam[i + 2];
      uint8_t pidx = attr & 0b11;
      bool flip_horiz = attr & 0b01000000;
      bool flip_vert = attr & 0b10000000;

      auto palette = spritePalette(pidx);
      auto bank = registers.spritePTableAddr();
      auto tile_base = bank + tile_idx * 16;

      for (int y = 0; y < 8; ++y) {
        auto lower = readByte(tile_base + y);
        auto upper = readByte(tile_base + y + 8);
        for (int x = 7; x >= 0; x--) {
          auto value = ((upper & 1) << 1) | (lower & 1);
          upper >>= 1;
          lower >>= 1;
          if (value == 0) {
            continue;
          }
          uint8_t rgb = palette[value];
          int px = sprite_x + x;
          int py = sprite_y + y;

          if (!flip_horiz && !flip_vert) {
            (void)px;
            (void)py;
          } else if (flip_horiz && flip_vert) {
            px = sprite_x + (7 - x);
            py = sprite_y + (7 - y);
          } else if (flip_horiz) {
            px = sprite_x + (7 - x);
          } else if (flip_vert) {
            py = sprite_y + (7 - y);
          }
          set_pixel(px, py, SystemPalette[rgb]);
        }
      }
    }
    return true;
  } else {
    return false;
  }
}

std::array<uint8_t, 4> PPU::bgPalette(uint16_t base, uint16_t tile_x,
                                      uint16_t tile_y) {
  uint16_t attribute_table_base = base + 0x3c0;
  uint8_t block_x = tile_x >> 2;
  uint8_t block_y = tile_y >> 2;
  auto at_entry = readByte(attribute_table_base + (block_y * 8 + block_x));
  uint8_t meta_x = (tile_x & 0x03) >> 1;
  uint8_t meta_y = (tile_y & 0x03) >> 1;
  uint8_t shift = (meta_y * 2 + meta_x) << 1;
  uint8_t pidx = ((at_entry >> shift) & 0b11) << 2;
  return {readByte(0x3f00), readByte(0x3f01 + pidx), readByte(0x3f02 + pidx),
          readByte(0x3f03 + pidx)};
}

inline std::array<uint8_t, 4> PPU::spritePalette(uint8_t pidx) {
  pidx <<= 2;
  return {0, readByte(0x3f11 + pidx), readByte(0x3f12 + pidx),
          readByte(0x3f13 + pidx)};
}

void PPU::set_pixel(uint8_t x, uint8_t y, std::array<uint8_t, 3> const &rgb) {
  int pi = y * WIDTH + x;
  if (pi < framebuf_.size()) {
    for (int i = 0; i < 3; ++i) {
      framebuf_[pi][i] = rgb[i];
    }
  }
}

void PPU::tick() {
  ++cycle_;
  if (cycle_ == 341) {
    if (registers.showSprites() && oam[0] == scanline_ && oam[3] <= cycle_) {
      registers.setSpriteZeroHit(true);
    }
    ++scanline_;
    cycle_ = 0;
  }
}

void PPU::showPatternTable() {
  uint8_t x = 0;
  uint8_t y = 0;
  for (int bank = 0; bank < 2; ++bank) {
    for (int i = 0; i < 256; ++i) {
      showTile(x, y, bank, i);
      x += 8;
      if (x == 0) {
        y += 8;
      }
    }
  }
}

void PPU::showTile(uint8_t x, uint8_t y, uint8_t bank, uint8_t tile) {
  AddressT base = bank * 0x1000;
  base += tile * 0x10;
  for (int yi = y; yi < y + 8; ++yi) {
    uint8_t lower = readByte(base);
    uint8_t upper = readByte(base + 8);
    for (int xi = x + 7; xi >= x; --xi) {
      uint8_t pi = ((upper & 1) << 1) | (lower & 1);
      upper >>= 1;
      lower >>= 1;
      // TODO(oren): these colors are totally arbitrary
      // need to hook this up to the actual palette
      switch (pi) {
      case 0:
        set_pixel(xi, yi, SystemPalette[0x01]);
        break;
      case 1:
        set_pixel(xi, yi, SystemPalette[0x23]);
        break;
      case 2:
        set_pixel(xi, yi, SystemPalette[0x27]);
        break;
      case 3:
        set_pixel(xi, yi, SystemPalette[0x30]);
        break;
      }
    }
    ++base;
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
