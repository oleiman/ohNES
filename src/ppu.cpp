#include "ppu.hpp"
#include "util.hpp"

#include <algorithm>
#include <bitset>
#include <fstream>

namespace vid {

std::array<std::array<uint8_t, 3>, 64> PPU::SystemPalette = {};

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
      scanline_ = 0;
    }

    if (!rendering()) {
      vBlankLine();
    } else if (scanline_ < 240) {
      visibleLine();
    } else if (scanline_ == 240) {
    } else if (scanline_ < 261) {
      vBlankLine();
    } else if (scanline_ == 261) {
      // pre-render scanline
      // TODO(oren): not sure this is exactly the same as a visible line.
      // memory reads should be the same (to prepare first two tiles of next
      // line) but it doesn't make sense to actually render.
      visibleLine();
      if (cycle_ == 260) {
      } else if (280 <= cycle_ && cycle_ <= 304) {
        registers_.syncScrollY();
      }
    }
    tick();
  }
}

void PPU::visibleLine(bool pre_render) {

  // NOTE(oren): this would indicate either:
  //    a) a timing bug in the emulation (PPU or CPU) or
  //    b) a software bug in the game

  // TODO(oren): I still think these shouldn't be hitting in SMB3
  // likely I'm simply not monitoring the address bus property (16-pix sprites)
  if (registers_.writePending() && !pre_render) {
    registers_.clearWritePending();
    registers_.incHorizScroll();
    registers_.incVertScroll();
    // std::cout << "S: " << +scanline_ << ", C: " << +cycle_
    //           << ", Addr: " << std::hex << registers_.vRamAddr() << std::dec
    //           << std::endl;
    // throw std::runtime_error("CPU writing during non-VBLANK line " +
    //                          std::to_string(scanline_));
  } else if (registers_.readPending() && !pre_render) {
    registers_.clearReadPending();
    registers_.incHorizScroll();
    registers_.incVertScroll();
    // std::cout << "S: " << +scanline_ << ", C: " << +cycle_
    //           << ", Addr: " << std::hex << registers_.vRamAddr() << std::dec
    //           << std::endl;
    // throw std::runtime_error("CPU reading during non-VBLANK line " +
    //                          std::to_string(scanline_));
  }

  handleBackground(pre_render);
  handleSprites(pre_render);
}

void PPU::handleBackground(bool pre_render) {
  // NOTE(oren): do nothing on cycle 0
  if (1 <= cycle_ && cycle_ <= 256) { // Data for current scanline
    int dot_x = cycle_ - 1;
    int dot_y = scanline_;

    LoadBackground();
    if (registers_.showBackground() && !pre_render) {
      renderBgPixel(dot_x, dot_y);
    }
    backgroundSR_.Shift();

    if (cycle_ == 256) {
      registers_.incVertScroll();
    }
  } else if (cycle_ == 257) {
    registers_.syncScrollX();
  } else if (321 <= cycle_ && cycle_ <= 336) {
    LoadBackground();
    backgroundSR_.Shift();
  } else if (cycle_ == 337 || cycle_ == 339) {
    // garbage nametable reads (timing for mmc5?)
    fetchNametable();
  }
}

void PPU::handleSprites(bool pre_render) {

  // rendering
  if (1 <= cycle_ && cycle_ < 257) {
    int dot_x = cycle_ - 1;
    int dot_y = scanline_;
    if (registers_.showSprites() && !pre_render) {
      renderSpritePixel(dot_x, dot_y);
    }
  }

  // evaluation
  if (1 <= cycle_ && cycle_ <= 64) {
    clearOam();
  } else if (65 <= cycle_ && cycle_ <= 256) {
    evaluateSprites();
  } else if (257 <= cycle_ && cycle_ <= 320) {
    // fetch sprites
    fetchSprites();
    if (cycle_ == 260) {
      mapper_.setPpuABus(0x1000);
    }
  } else if (cycle_ == 321) {
    // move sprites from the staging area ("latches") into the rendering area
    // ("registers")
    mapper_.setPpuABus(0x0000);
    std::copy(std::begin(sprites_staging_), std::end(sprites_staging_),
              std::begin(sprites_));
  }

  return;
}

void PPU::clearOam() {
  if (cycle_ % 2 == 1) {
    assert((cycle_ >> 1) < secondary_oam_.size());
    secondary_oam_[cycle_ >> 1] = 0xFF;
  }
  if (cycle_ % 8 == 1) {
    sprites_staging_[cycle_ >> 3].v = 0;
  }
  oam_n_ = 0;
  oam_m_ = 0;
  sec_oam_n_ = 0;
  sec_oam_write_enable_ = true;
}

void PPU::evaluateSprites() {
  auto y_coord = secondary_oam_[4 * sec_oam_n_];

  // TODO(oren): refactor for sprite overflow
  if (sec_oam_write_enable_) {
    if (oam_m_ == 0) {
      assert(4 * sec_oam_n_ < secondary_oam_.size());
      secondary_oam_[4 * sec_oam_n_] = oam_[4 * oam_n_];
      oam_m_++;
    } else if (y_coord <= scanline_ &&
               scanline_ < (y_coord + registers_.spriteSize())) {
      assert(4 * sec_oam_n_ + oam_m_ < secondary_oam_.size());
      secondary_oam_[4 * sec_oam_n_ + oam_m_] = oam_[4 * oam_n_ + oam_m_];
      sprites_staging_[sec_oam_n_].s.idx = oam_n_;
      ++oam_m_;
      if (oam_m_ == 4) {
        sec_oam_n_++;
        oam_n_++;
        oam_m_ = 0;
      }
    } else {
      // TODO(oren): actually supposed to leave this as the out of range y-coord
      assert(4 * sec_oam_n_ < secondary_oam_.size());
      secondary_oam_[4 * sec_oam_n_] = 0xFF;
      oam_n_++;
      oam_m_ = 0;
    }
  } else if (oam_n_ < (oam_.size() >> 2)) {
    oam_n_++;
  }

  if (sec_oam_n_ >= sprites_staging_.size() || oam_n_ >= (oam_.size() >> 2)) {
    sec_oam_write_enable_ = false;
  }
}

void PPU::fetchSprites() {
  static uint8_t sprite_y;
  static uint8_t tile_idx;
  static uint8_t idx = 0;

  // HACK(oren): seems like we miss cycles occasionally
  // I've noticed this in the switch stmt below as well...not sure why
  if (cycle_ < 264) {
    idx = 0;
  }

  bool dummy = secondary_oam_[4 * idx] >= 0xEF;
  Sprite &sprite = (dummy ? dummy_sprite_ : sprites_staging_[idx]);

  uint8_t step = cycle_ & 0b111;
  switch (step) {
  case 0b001:
    readByte(registers_.spritePTableAddr(0));
    sprite_y = secondary_oam_[4 * idx];
    break;
  case 0b010:
    tile_idx = secondary_oam_[4 * idx + 1];
    break;
  case 0b011:
    readByte(registers_.spritePTableAddr(0));
    sprite.s.attrs.v = secondary_oam_[4 * idx + 2];
    break;
  case 0b100:
    sprite.s.xpos = secondary_oam_[4 * idx + 3];
    break;
  case 0b101:
  case 0b111: {
    int tile_y = scanline_ - sprite_y;
    if (sprite.s.attrs.s.v_flip) {
      tile_y = registers_.spriteSize() - 1 - tile_y;
    }
    auto bank = registers_.spritePTableAddr(tile_idx);
    auto tmp_idx = tile_idx;
    if (registers_.spriteSize() == 16) {
      tmp_idx &= 0xFE;
    }
    auto tile_base = bank + (tmp_idx * 16);
    // NOTE(oren): If we're looking at the bottom half of an 8x16 sprite,
    // skip to the next tile (top/bottom halves are arranged consecutively
    // in pattern table RAM)
    if (tile_y >= 8) {
      tile_y -= 8;
      tile_base += 16;
    }
    auto &v = (step == 0b101 ? sprite.s.tile_lo : sprite.s.tile_hi);
    uint8_t off = (step == 0b101 ? 0 : 8);
    v = readByte(tile_base + tile_y + off);
    if (!sprite.s.attrs.s.h_flip) {
      util::reverseByte(v);
    }
    break;
  }
  case 0b000:
    if (!dummy) {
      ++idx;
    }
    if (idx >= sprites_staging_.size() && cycle_ < 320) {
      idx = sprites_staging_.size() - 1;
    }
    break;
  default:
    break;
  }
}

void PPU::fetchSprites_old() {
  std::copy(std::begin(sprites_staging_), std::end(sprites_staging_),
            std::begin(sprites_));

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
    if (!sprite.s.attrs.s.h_flip) {
      util::reverseByte(sprite.s.tile_lo);
      util::reverseByte(sprite.s.tile_hi);
    }
    sprites_[i >> 2].v |= sprite.v;
  }
}

void PPU::LoadBackground() {
  switch (cycle_ & 0b111) {
  case 0b000:
    registers_.incHorizScroll();
    break;
  case 0b001:
    backgroundSR_.Load();
    fetchNametable();
    break;
  case 0b011:
    fetchAttribute();
    break;
  case 0b101:
    fetchPattern(PTOff::LOW);
    break;
  case 0b111:
    fetchPattern(PTOff::HIGH);
    break;
  default:
    break;
  }
}

void PPU::fetchNametable() {
  uint8_t pixel_x = registers_.scrollX_coarse();
  uint8_t pixel_y = registers_.scrollY();
  AddressT base = registers_.baseNametableAddr();
  int tile_x = pixel_x >> 3;
  int tile_y = pixel_y >> 3;
  int nametable_i = tile_y * 32 + tile_x;
  nametable_reg = readByte(base + nametable_i);
}

void PPU::fetchAttribute() {
  uint8_t pixel_x = registers_.scrollX_coarse();
  uint8_t pixel_y = registers_.scrollY();
  AddressT base = registers_.baseNametableAddr();
  int tile_x = pixel_x >> 3;
  int tile_y = pixel_y >> 3;
  int block_x = pixel_x >> 5;
  int block_y = pixel_y >> 5;
  uint16_t attr_table_base = base + 0x3c0;
  auto at_entry = readByte(attr_table_base + (block_y * 8 + block_x));
  uint8_t meta_x = (tile_x & 0x03) >> 1;
  uint8_t meta_y = (tile_y & 0x03) >> 1;
  uint8_t shift = (meta_y * 2 + meta_x) << 1;
  uint8_t val = (at_entry >> shift) & 0b11;
  backgroundSR_.attr_lo.Latch(0xFF * (val & 0b1));
  backgroundSR_.attr_hi.Latch(0xFF * ((val >> 1) & 0b1));
}

void PPU::fetchPattern(PTOff plane) {
  uint8_t pixel_y = registers_.scrollY();
  int y = pixel_y % 8;
  auto tile = static_cast<uint16_t>(nametable_reg);
  auto tile_base = registers_.backgroundPTableAddr() + tile * 16;
  auto val = readByte(tile_base + y + static_cast<uint16_t>(plane));
  util::reverseByte(val);
  switch (plane) {
  case PTOff::LOW:
    backgroundSR_.pattern_lo.Latch(val);
    break;
  case PTOff::HIGH:
    backgroundSR_.pattern_hi.Latch(val);
    break;
  }
}

void PPU::renderBgPixel(int abs_x, int abs_y) {
  uint8_t fine_x = registers_.scrollX_fine();
  auto palette = bgPalette();
  auto lower = backgroundSR_.pattern_lo.value >> fine_x;
  auto upper = backgroundSR_.pattern_hi.value >> fine_x;
  auto value = ((upper & 0x01) << 1) | (lower & 0x01);
  bg_zero_ = (value == 0);
  set_pixel(abs_x, abs_y, SystemPalette[palette[value]]);
}

void PPU::renderSpritePixel(int abs_x, int abs_y) {
  // NOTE(oren): ensure that the first opaque sprite pixel takes priority
  // but also that all sprites have their x positions advanced as appropriate.
  bool filled = false;
  for (auto &sprite : sprites_) {
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
          // TODO(oren): should this be set outside the conditional block?
        }
        filled = true;
      }
    }
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

bool PPU::rendering() {
  return registers_.showBackground() || registers_.showSprites();
}

std::array<uint8_t, 4> PPU::bgPalette() {
  auto fine_x = registers_.scrollX_fine();
  auto lo = (backgroundSR_.attr_lo.value >> fine_x) & 0b1;
  auto hi = (backgroundSR_.attr_hi.value >> fine_x) & 0b1;

  uint8_t attr = ((hi << 1) | lo) & 0b11;
  uint8_t pidx = attr << 2;

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

} // namespace vid
