#include "ppu.hpp"
#include "util.hpp"

#include <algorithm>
#include <bitset>
#include <fstream>

using std::array;
using std::to_string;

const char PALETTE_PATH[] = "../data/2c02.palette";

namespace {
void LoadSystemPalette(std::string const &fname,
                       std::array<std::array<uint8_t, 3>, 64> &p) {
  std::ifstream istrm(fname, std::ios::binary);
  int i = 0;
  int entry;
  while (istrm >> entry) {
    p[i / 3][i % 3] = entry;
    ++i;
  }
}
} // namespace

namespace vid {

PPU::PPU(mapper::NESMapper &mapper, Registers &registers,
         std::array<uint8_t, 256> &oam)
    : mapper_(mapper), registers_(registers), oam_(oam) {
  // LoadSystemPalette(PALETTE_PATH, SystemPalette);
}

void PPU::step(uint16_t cycles, bool &nmi) {
  if (registers_.handleNmi()) {
    nmi = true;
  }
  cycles += registers_.oamCycles();
  while (cycles-- > 0) {
    if (cycle_ == 1 && scanline_ == 241) {
      if (registers_.vBlankNMI()) {
        nmi = true;
      }
      registers_.setVBlankStarted();
      registers_.setSpriteZeroHit(false);
    } else if (scanline_ >= 262) {
      registers_.clearVBlankStarted();
      registers_.setSpriteZeroHit(false);
      scanline_ = 0;
    }

    if (!(registers_.showBackground() || registers_.showSprites())) {
      vBlankLine();
      // tick();
    } else if (scanline_ < 240) {
      visibleLine();
    } else if (scanline_ == 240) {
    } else if (scanline_ < 261) { // VBlank lines
      vBlankLine();
    } else if (scanline_ == 261) {
      // TODO(oren): pre-render scanline?
    }
    tick();
  }
}

void PPU::visibleLine() {

  // NOTE(oren): this would indicate either:
  //    a) a timing bug in the emulation (PPU or CPU) or
  //    b) a software bug in the game
  if (registers_.writePending()) {
    throw std::runtime_error("CPU writing during non-VBLANK line");
  } else if (registers_.readPending()) {
    throw std::runtime_error("CPU reading during non-VBLANK line");
  }

  // if (cycle_ == 0) {
  //   // do nothing
  // } else
  if (cycle_ >= 1 && cycle_ < 257) { // Data for current scanline
    int pixel_absolute_x = cycle_ - 1;
    int pixel_absolute_y = scanline_;

    if (registers_.showBackground()) {
      auto nt = selectNametable(pixel_absolute_x, pixel_absolute_y);
      renderBgPixel(nt.base, nt.view, pixel_absolute_x, pixel_absolute_y);
    }

    if (registers_.showSprites()) {
      renderSpritePixel(pixel_absolute_x, pixel_absolute_y);
    }

    // TODO(oren): shift registers will make it easier to account for hitting
    // the visible portion of Sprite Zero
    // HACK(oren):  "scanline - 8"
    if (registers_.showSprites() && oam_[0] == scanline_ - 8 &&
        oam_[3] <= cycle_) {
      registers_.setSpriteZeroHit(true);
    }

  } else if (cycle_ == 340) {
    // NOTE(oren): Pack sprite evaluation logic into last cycle on the scanline
    // for simplicity. Regain 5 sanity points and cure "Terror" effect.
    evaluateSprites();
  }
  // NOTE(oren): the last few branches are junk that
  // else if (cycle_ < 321) {
  //                            // I'll probably never use
  // } else if (cycle_ < 336) {
  //   // TODO(oren): load first 2 tiles for next scanline into shift registers
  // } else {
  //   // TODO(oren): throwaway reads (cycles 337 - 340)
  // }
}

// NOTE(oren): this only allows for scrolling in one direction at a time. that
// is, if we're not scrolling in the x direction, we're scrolling in the y
// direction. additionally, there's no distinction made between coarse and fine
// scroll bits. instead, we just treat the X and Y scroll bytes geometrically,
// describing the portion of the visible screen that should map to each
// nametable.
PPU::Nametable PPU::selectNametable(int x, int y) {

  auto [primary, secondary] = constructNametables();

  // TODO(oren): these shouldn't change outside of VBLANK, so we shouldn't
  // have to construct them every cycle...
  // on the other hand, why carry around the extra state?

  if (primary.describesPixel(x, y)) {
    return primary;
  } else if (secondary.describesPixel(x, y)) {
    return secondary;
  } else {
    std::stringstream ss;
    ss << "Pixel (" << +x << ", " << +y << ") out of range?" << std::endl;
    ss << "Primary " << primary << std::endl;
    ss << "Secondary " << secondary;
    throw std::runtime_error(ss.str());
  }
}

std::pair<PPU::Nametable, PPU::Nametable> PPU::constructNametables() {
  if (!registers_.scrollY()) {
    Nametable primary = {
        registers_.baseNametableAddr(),
        {
            registers_.scrollX(),
            0,
            255,
            239,
            {
                -registers_.scrollX(),
                0,
            },
        },
    };
    Nametable secondary = {
        static_cast<AddressT>(0x2000 +
                              ((primary.base - 0x2000 + 0x400) % 0x800)),
        {
            0,
            0,
            registers_.scrollX(),
            239,
            {
                255 - registers_.scrollX(),
                0,
            },
        },
    };
    return std::make_pair<>(primary, secondary);
  } else {
    Nametable primary = {
        registers_.baseNametableAddr(),
        {
            0,
            registers_.scrollY(),
            255,
            239,
            {
                0,
                -registers_.scrollY(),
            },
        },
    };
    Nametable secondary = {
        static_cast<AddressT>(0x2000 +
                              ((primary.base - 0x2000 + 0x400) % 0x800)),
        {
            0,
            0,
            255,
            registers_.scrollY(),
            {
                0,
                239 - registers_.scrollY(),
            },
        },
    };
    return std::make_pair<>(primary, secondary);
  }
}

void PPU::renderBgPixel(uint16_t nt_base, View const &view, int abs_x,
                        int abs_y) {
  uint8_t pixel_x = abs_x - view.shift.x;
  uint8_t pixel_y = abs_y - view.shift.y;
  int x = pixel_x % 8;
  int y = pixel_y % 8;
  int tile_x = pixel_x / 8;
  int tile_y = pixel_y / 8;
  auto palette = bgPalette(nt_base, tile_x, tile_y);
  int nametable_i = tile_y * 32 + tile_x;
  auto tile = static_cast<uint16_t>(readByte(nt_base + nametable_i));
  auto tile_base = registers_.backgroundPTableAddr() + tile * 16;
  auto lower = readByte(tile_base + y) >> (7 - x);
  auto upper = readByte(tile_base + y + 8) >> (7 - x);
  auto value = ((upper & 0x01) << 1) | (lower & 0x01);
  bg_nonzero_ = (value != 0);
  set_pixel(abs_x, abs_y, SystemPalette[palette[value]]);
}

// TODO(oren): deal with sprite/background priority
void PPU::renderSpritePixel(int abs_x, int abs_y) {
  sprite_nonzero_ = false;
  for (int i = 0; i < 8 && sprite_xpos[i] > 0; ++i) {
    uint8_t attr = sprite_attrs[i];
    uint8_t pidx = attr & 0b11;
    auto palette = spritePalette(pidx);
    auto &sprite_x = sprite_xpos[i];
    auto &lower = sprite_tiles_l[i];
    auto &upper = sprite_tiles_h[i];
    if (abs_x == sprite_x && (lower > 0 || upper > 0)) {
      uint16_t value = ((upper & 0x01) << 1) | (lower & 0x01);
      lower >>= 1;
      upper >>= 1;
      ++sprite_x;
      if (value > 0) {
        sprite_nonzero_ = true;
        set_pixel(abs_x, abs_y, SystemPalette[palette[value]]);
      }
    }
  }
}

void PPU::evaluateSprites() {
  // TODO(oren): some tricks might depend on spreading this out over the correct
  // cycles
  int next_scanline = scanline_ + 1;
  secondary_oam_.fill(0xFF);
  for (int n1 = 0, n2 = 0; n1 < oam_.size() && n2 < secondary_oam_.size();
       n1 += 4) {
    if (next_scanline >= oam_[n1] && next_scanline < (oam_[n1] + 8)) {
      for (int m = 0; m < 4; ++m) {
        secondary_oam_[n2 + m] = oam_[n1 + m];
      }
      n2 += 4;
    }
  }
  sprite_tiles_l.fill(0);
  sprite_tiles_h.fill(0);
  sprite_xpos.fill(0);
  sprite_attrs.fill(0);
  // TODO(oren): secondary oam entries could/should be structs...less error
  // prone and less horrible to read.
  for (int i = 0; secondary_oam_[i] != 0xFF && i < secondary_oam_.size();
       i += 4) {
    uint8_t tile_idx = secondary_oam_[i + 1];
    uint8_t sprite_x = secondary_oam_[i + 3];
    uint8_t sprite_y = secondary_oam_[i];
    uint8_t attr = secondary_oam_[i + 2];
    bool flip_vert = attr & 0b10000000;
    bool flip_horiz = attr & 0b01000000;
    auto bank = registers_.spritePTableAddr();
    auto tile_base = bank + tile_idx * 16;
    int y = next_scanline - sprite_y;
    if (flip_vert) {
      y = 7 - y;
    }
    sprite_tiles_l[i / 4] = readByte(tile_base + y);
    sprite_tiles_h[i / 4] = readByte(tile_base + y + 8);
    if (!flip_horiz) {
      util::reverseByte(sprite_tiles_l[i / 4]);
      util::reverseByte(sprite_tiles_h[i / 4]);
    }
    sprite_xpos[i / 4] = sprite_x;
    sprite_attrs[i / 4] = attr;
  }
}

void PPU::vBlankLine() {
  bool odd = cycle_ & 0x01;

  // TODO(oren): service CPU memory requests
  // again only on odd cycles to account for 2-cycle cost. may need to look into
  // this a bit more
  if (odd) {
    if (registers_.readPending()) {
      // std::cout << "service read" << std::endl;
      auto addr = registers_.vRamAddr();
      registers_.putData(readByte(addr));
    } else if (registers_.writePending()) {
      auto addr = registers_.vRamAddr();
      auto data = registers_.getData();
      writeByte(addr, data);
    }
  }
}

bool PPU::render() {
  return registers_.showBackground() || registers_.showSprites();
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
  return {
      readByte(0x3f00 + pidx),
      readByte(0x3f01 + pidx),
      readByte(0x3f02 + pidx),
      readByte(0x3f03 + pidx),
  };
}

inline std::array<uint8_t, 4> PPU::spritePalette(uint8_t pidx) {
  pidx <<= 2;
  return {
      0,
      readByte(0x3f11 + pidx),
      readByte(0x3f12 + pidx),
      readByte(0x3f13 + pidx),
  };
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

std::ostream &operator<<(std::ostream &os, PPU::Nametable const &in) {
  std::stringstream ss;
  ss << std::hex;
  ss << "Base: 0x" << in.base << std::endl;
  ss << std::dec;
  ss << "View: " << std::endl;
  ss << "\tMin: (" << +in.view.x_min << ", " << +in.view.y_min << ")"
     << std::endl;
  ss << "\tMax: (" << +in.view.x_max << ", " << +in.view.y_max << ")"
     << std::endl;
  ss << "\tShift: " << std::endl;
  ss << "\t\tX: " << +in.view.shift.x << std::endl;
  ss << "\t\tY: " << +in.view.shift.y << std::endl;
  os << ss.str();
  return os;
}

} // namespace vid
