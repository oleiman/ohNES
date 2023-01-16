#include "ppu.hpp"
#include "util.hpp"

#include <algorithm>
#include <bitset>
#include <fstream>

namespace vid {

std::array<std::array<uint8_t, 3>, 64> PPU::SystemPalette = {};

// TODO(oren): how to work this out for e.g. zelda
std::unordered_map<PPU::Scroll,
                   std::unordered_map<PPU::AddressT, PPU::AddressT>>
    PPU::SecondaryNTMap = {{
        // HORIZONTAL scroll (vertical mirror)
        {
            Scroll::HORIZONTAL,
            {{0x2000, 0x2400},
             {0x2400, 0x2000},
             {0x2800, 0x2C00},
             {0x2C00, 0x2800}},
        },
        // VERTICAL scroll (horizontal mirror)
        {
            Scroll::VERTICAL,
            {{0x2000, 0x2800},
             {0x2800, 0x2000},
             {0x2400, 0x2C00},
             {0x2C00, 0x2400}},
        },
    }};

void LoadSystemPalette(const std::string &fname) {
  std::ifstream istrm(fname, std::ios::binary);
  int i = 0;
  std::array<int, 3> rgb;
  while (istrm >> rgb[0] >> rgb[1] >> rgb[2]) {
    std::copy(std::begin(rgb), std::end(rgb),
              std::begin(vid::PPU::SystemPalette[i]));
    ++i;
  }
}

PPU::PPU(mapper::NESMapper &mapper, Registers &registers,
         std::array<uint8_t, 256> &oam)
    : mapper_(mapper), registers_(registers), oam_(oam) {}

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
    } else if (scanline_ == 261 && cycle_ == 1) {
      ClearSpriteZeroHit();
      registers_.clearVBlankStarted();
    } else if (scanline_ >= 262) {
      // registers_.clearVBlankStarted();
      scanline_ = 0;
      renderedSprite_ = false;
    }

    if (!render()) {
      vBlankLine();
    } else if (scanline_ < 240) {
      visibleLine();
    } else if (scanline_ == 240) {
    } else if (scanline_ < 261) {
      vBlankLine();
    } else if (scanline_ == 261) {
      // TODO(oren): pre-render scanline?
      if (cycle_ == 260) {
        // if (mapper_.setPpuABus(0x1000)) {
        //   // std::cout << "fired irq from pre-render scanline " << +scanline_
        //   //           << std::endl;
        // }
        // mapper_.setPpuABus(0x0000);
      } else if (280 <= cycle_ && cycle_ <= 304) {
        registers_.syncScrollY();
      }
    }
    tick();
  }
}

void PPU::visibleLine() {

  // NOTE(oren): this would indicate either:
  //    a) a timing bug in the emulation (PPU or CPU) or
  //    b) a software bug in the game
  if (registers_.writePending()) {
    std::cout << "S: " << +scanline_ << ", C: " << +cycle_
              << ", Addr: " << std::hex << registers_.vRamAddr() << std::dec
              << std::endl;
    throw std::runtime_error("CPU writing during non-VBLANK line " +
                             std::to_string(scanline_));
  } else if (registers_.readPending()) {
    std::cout << "S: " << +scanline_ << ", C: " << +cycle_
              << ", Addr: " << std::hex << registers_.vRamAddr() << std::dec
              << std::endl;
    throw std::runtime_error("CPU reading during non-VBLANK line " +
                             std::to_string(scanline_));
  }

  // NOTE(oren): do nothing on cycle 0
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
  } else if (cycle_ == 257) {
    registers_.syncScrollX();
  } else if (cycle_ == 260) {
    // NOTE(oren): hack for mmc3, required because we calculate nametable stuff
    // on every cycle and evaluate sprites wholesale at the very end of the
    // scanline
    // if (mapper_.setPpuABus(0x1000)) {
    //   std::cout << "fired irq at scanline " << +scanline_ << std::endl;
    // }
    // mapper_.setPpuABus(0x0000);
  } else if (cycle_ == 340) {
    // NOTE(oren): Pack sprite evaluation logic into last cycle on the
    // scanline for simplicity. Regain 5 sanity points and cure "Terror"
    // effect.
    evaluateSprites();
  }
}

// NOTE(oren): this only allows for scrolling in one direction at a time. that
// is, if we're not scrolling in the x direction, we're scrolling in the y
// direction. additionally, there's no distinction made between coarse and fine
// scroll bits. instead, we just treat the X and Y scroll bytes geometrically,
// describing the portion of the visible screen that should map to each
// nametable.
PPU::Nametable PPU::selectNametable(int x, int y) {

  auto [primary, secondary] = constructNametables();

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
  if (scrollType() == Scroll::HORIZONTAL) {
    Nametable primary = {
        .base = registers_.baseNametableAddr(),
        .view =
            {
                .x_min = registers_.scrollX(),
                .y_min = 0,
                .x_max = 255,
                .y_max = 239,
                .shift =
                    {
                        .x = -registers_.scrollX(),
                        .y = 0,
                    },
            },
    };
    Nametable secondary = {
        .base = SecondaryNTMap[scrollType()][primary.base],
        .view =
            {
                .x_min = 0,
                .y_min = 0,
                .x_max = registers_.scrollX(),
                .y_max = 239,
                .shift =
                    {
                        .x = 255 - registers_.scrollX(),
                        .y = 0,
                    },
            },
    };
    return std::make_pair<>(primary, secondary);
  } else if (scrollType() == Scroll::VERTICAL) {
    Nametable primary = {
        .base = registers_.baseNametableAddr(),
        .view =
            {
                .x_min = 0,
                .y_min = registers_.scrollY(),
                .x_max = 255,
                .y_max = 239,
                .shift =
                    {
                        .x = 0,
                        .y = -registers_.scrollY(),
                    },
            },
    };
    Nametable secondary = {
        .base = SecondaryNTMap[scrollType()][primary.base],
        // static_cast<AddressT>(0x2000 + ((primary.base - 0x2000 + 0x800))),
        .view =
            {
                .x_min = 0,
                .y_min = 0,
                .x_max = 255,
                .y_max = registers_.scrollY(),
                .shift =
                    {
                        .x = 0,
                        .y = 239 - registers_.scrollY(),
                    },
            },
    };
    return std::make_pair<>(primary, secondary);
  } else { // single table (ignore scroll?)
    Nametable primary = {
        .base = registers_.baseNametableAddr(),
        .view =
            {
                .x_min = 0,
                .y_min = 0,
                .x_max = 255,
                .y_max = 239,
                .shift =
                    {
                        .x = 0,
                        .y = 0,
                    },
            },
    };
    return std::make_pair<>(primary, primary);
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
  bg_zero_ = (value == 0);
  set_pixel(abs_x, abs_y, SystemPalette[palette[value]]);
}

void PPU::renderSpritePixel(int abs_x, int abs_y) {
  // NOTE(oren): ensure that the first opaque sprite pixel takes priority
  // but also that all sprites have their x positions advanced as appropriate.
  bool filled = false;
  for (auto &sprite : sprites_) {
    if (sprite.s.xpos == 0)
      break;
    auto palette = spritePalette(sprite.s.attrs.s.palette_i);
    if (abs_x == sprite.s.xpos && (sprite.s.tile_lo || sprite.s.tile_hi)) {
      uint16_t value =
          ((sprite.s.tile_hi & 0x01) << 1) | (sprite.s.tile_lo & 0x01);
      sprite.s.tile_lo >>= 1;
      sprite.s.tile_hi >>= 1;
      ++sprite.s.xpos;

      if (value > 0 && sprite.s.idx == 0x00 && !bg_zero_) {
        SetSpriteZeroHit();
      }

      if (value > 0 && !filled) {
        if (Priority(sprite.s.attrs.s.priority) == Priority::FG || bg_zero_) {
          set_pixel(abs_x, abs_y, SystemPalette[palette[value]]);
        }
        // TODO(oren): should this be set inside the conditional block?
        filled = true;
      }
    }
  }
}

void PPU::evaluateSprites() {
  // TODO(oren): some tricks might depend on spreading this out over the
  // correct cycles
  // NOTE(oren): sprites are delayed by one scanline, so we use
  // the _previous_ scanline index to check against each sprite's y index
  secondary_oam_.fill(0xFF);
  sprites_.fill({.v = 0});

  for (int n1 = 0, n2 = 0; n1 < oam_.size() && n2 < secondary_oam_.size();
       n1 += 4) {
    if (scanline_ >= oam_[n1] &&
        scanline_ < (oam_[n1] + registers_.spriteSize())) {
      std::copy(std::begin(oam_) + n1, std::begin(oam_) + n1 + 4,
                std::begin(secondary_oam_) + n2);
      sprites_[n2 >> 2].s.idx = n1 >> 2;

      n2 += 4;
    }
  }

  // TODO(oren): whole range 0xEF - 0xFF is off limits
  for (int i = 0; i < secondary_oam_.size(); i += 4) {
    // pretty sure setting byte 0 to EF from program code just means
    // sprite_y will never sit on a visible scanline, so this check may
    // not actually be necessary
    if (secondary_oam_[i] >= 0xEF) {
      break;
    }
    Sprite sprite = {.v = 0};
    uint8_t sprite_y = secondary_oam_[i];
    uint8_t tile_idx = secondary_oam_[i + 1];
    sprite.s.attrs.v = secondary_oam_[i + 2];
    sprite.s.xpos = secondary_oam_[i + 3];

    int tile_y = scanline_ - sprite_y;
    if (sprite.s.attrs.s.v_flip) {
      tile_y = registers_.spriteSize() - 1 - tile_y;
    }

    auto bank = registers_.spritePTableAddr(tile_idx);
    if (registers_.spriteSize() == 16) {
      tile_idx &= 0xFE;
    }

    auto tile_base = bank + (tile_idx * 16);
    // NOTE(oren): If we're looking at the bottom half of an 8x16 sprite,
    // skip to the next tile (top/bottom halves are arraned consecutively
    // in pattern table RAM)
    if (tile_y >= 8) {
      tile_y -= 8;
      tile_base += 16;
    }
    sprite.s.tile_lo = readByte(tile_base + tile_y);
    sprite.s.tile_hi = readByte(tile_base + tile_y + 8);
    renderedSprite_ = true;
    if (!sprite.s.attrs.s.h_flip) {
      util::reverseByte(sprite.s.tile_lo);
      util::reverseByte(sprite.s.tile_hi);
    }
    sprites_[i >> 2].v |= sprite.v;
  }
}

void PPU::vBlankLine() {
  bool odd = cycle_ & 0x01;

  // TODO(oren): service CPU memory requests
  // again only on odd cycles to account for 2-cycle cost. may need to look
  // into this a bit more
  if (odd) {
    if (registers_.readPending()) {
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

  // TODO(oren): don't allocate this every time
  return {
      readByte(0x3f00),
      readByte(0x3f01 + pidx),
      readByte(0x3f02 + pidx),
      readByte(0x3f03 + pidx),
  };
}

// TODO(oren): don't allocate this every time
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
    std::copy(std::begin(rgb), std::end(rgb), std::begin(framebuf_[pi]));
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
