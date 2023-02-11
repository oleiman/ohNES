#include "nes_debugger.hpp"
#include "system.hpp"
#include "util.hpp"

using vid::PPU;

namespace sys {

NESDebugger::NESDebugger(NES &console)
    : dbg::Debugger(false, false), console_(console) {}
const instr::Instruction &NESDebugger::step(const instr::Instruction &in,
                                            const cpu::CpuState &state,
                                            mem::Mapper &mapper) {

  instr_cache_.insert(in);
  return in;
}

std::ostream &operator<<(std::ostream &os, NESDebugger &dbg) {
  os << dbg.instr_cache_;
  return os;
}

void NESDebugger::draw_nametable() {
  for (int y = 0; y < 240; ++y) {
    uint8_t pattern_lo = 0;
    uint8_t pattern_hi = 0;
    std::array<uint8_t, 4> palette = {};
    for (int x = 0; x < 256; ++x) {
      int pixel_x = x;
      int pixel_y = y;

      // for each slice of 8 pixels, fetch
      //   - a nametable entry
      //   - an attribute
      //   - two bytes of pattern data (hi & lo)
      //   - a palette
      // TODO(oren): easier to write tile-wise? like the others...
      if ((pixel_x & 0b111) == 0) {
        // nametable fetch
        uint16_t nt_base = console_.ppu_registers_.BaseNTAddr[nt_select];
        int tile_x = pixel_x >> 3;
        int tile_y = pixel_y >> 3;
        int nt_i = tile_y * 32 + tile_x;
        uint8_t nt = ppu_read(nt_base + nt_i);

        // attribute fetch
        int block_x = tile_x >> 2;
        int block_y = tile_y >> 2;
        uint16_t attr_base = nt_base + 0x3c0;
        uint8_t at_entry = ppu_read(attr_base + (block_y * 8 + block_x));
        uint8_t meta_x = (tile_x & 0x03) >> 1;
        uint8_t meta_y = (tile_y & 0x03) >> 1;
        uint8_t shift = (meta_y * 2 + meta_x) << 1;
        uint8_t val = (at_entry >> shift) & 0b11;
        uint8_t attr = val & 0b11;

        // pattern fetches
        uint8_t y_fine = pixel_y & 0b111;
        auto tile = static_cast<uint16_t>(nt);
        auto tile_base =
            console_.ppu_registers_.backgroundPTableAddr() + tile * 16;
        pattern_lo = ppu_read(tile_base + y_fine);
        pattern_hi = ppu_read(tile_base + y_fine + 8);
        util::reverseByte(pattern_lo);
        util::reverseByte(pattern_hi);

        // palette fetch
        auto pidx = attr << 2;
        palette[0] = ppu_read(0x3f00);
        palette[1] = ppu_read(0x3f01 + pidx);
        palette[2] = ppu_read(0x3f02 + pidx);
        palette[3] = ppu_read(0x3f03 + pidx);
      }

      // render pixel
      auto fine_x = pixel_x & 0b111;
      auto lower = pattern_lo >> fine_x;
      auto upper = pattern_hi >> fine_x;
      auto value = ((upper & 0b1) << 1) | (lower & 0b1);
      auto c = palette[value] & 0x3F;
      set_pixel(x, y, PPU::SystemPalette[c]);
    }
  }
}

void NESDebugger::draw_sprites() {

  // evaluate sprites (primary OAM)
  const auto &oam = console_.ppu_oam_;
  uint8_t sprite_size = console_.ppu_registers_.spriteSize();
  uint8_t n_sprites = oam.size() >> 2;
  std::array<uint8_t, 4> palette = {};

  uint16_t vert_offset = 272;

  for (uint8_t i = 0; i < n_sprites; ++i) {
    uint16_t x = (i & 0b1111) * 8 * 2;
    uint16_t y = vert_offset + (i >> 4) * 16 * 2;
    uint8_t sprite_base = i << 2;
    // uint8_t sprite_y = oam[sprite_base];
    uint8_t tile_idx = oam[sprite_base + 1];
    PPU::Sprite sprite = {.s = {.attrs.v = oam[sprite_base + 2]}};
    uint8_t pidx = sprite.s.attrs.s.palette_i;
    palette[0] = ppu_read(0x3f00);
    palette[1] = ppu_read(0x3f11 + pidx);
    palette[2] = ppu_read(0x3f12 + pidx);
    palette[3] = ppu_read(0x3f13 + pidx);
    auto bank = console_.ppu_registers_.spritePTableAddr(tile_idx);
    if (sprite_size == 16) {
      tile_idx &= 0xFE;
    }
    auto tile_base = bank + (tile_idx * 16);
    for (int tile_y = 0; tile_y < sprite_size; ++tile_y) {
      int y_tmp = tile_y;
      uint16_t base_tmp = tile_base;
      if (y_tmp >= 8) {
        y_tmp -= 8;
        base_tmp += 16;
      }
      uint8_t tile_lo = ppu_read(base_tmp + y_tmp);
      uint8_t tile_hi = ppu_read(base_tmp + y_tmp + 8);
      if (sprite.s.attrs.s.h_flip) {
        util::reverseByte(tile_lo);
        util::reverseByte(tile_hi);
      }
      for (int tile_x = 0; tile_x < 8; ++tile_x) {
        uint16_t value = ((tile_hi & 0b1) << 1) | (tile_lo & 0b1);
        tile_lo >>= 1;
        tile_hi >>= 1;
        auto c = palette[value] & 0x3f;
        set_pixel(x + tile_x, y + tile_y, PPU::SystemPalette[c]);
      }
    }
  }
}

void NESDebugger::draw_ptables() {
  //
  std::array<uint8_t, 4> palette = {
      ppu_read(0x3f00),
      ppu_read(0x3f01 + palette_idx()),
      ppu_read(0x3f02 + palette_idx()),
      ppu_read(0x3f03 + palette_idx()),
  };
  uint16_t horiz_off = 256 + 8;
  uint16_t vert_off = 0;
  for (uint16_t base = 0x0000; base < 0x2000; base += 0x1000) {
    uint16_t x_off = horiz_off + (base >> 5);
    for (int i = 0; i < 16 * 16; ++i) {
      int x = (i & 0b1111) * 8 + x_off;
      int y = (i >> 4) * 8 + vert_off;
      uint16_t tile_base = base + i * 16;
      for (uint8_t tile_y = 0; tile_y < 8; ++tile_y) {
        uint8_t pattern_lo = ppu_read(tile_base + tile_y);
        uint8_t pattern_hi = ppu_read(tile_base + tile_y + 8);
        util::reverseByte(pattern_lo);
        util::reverseByte(pattern_hi);
        for (uint8_t tile_x = 0; tile_x < 8;
             ++tile_x, pattern_lo >>= 1, pattern_hi >>= 1) {
          auto value = ((pattern_hi & 0b1) << 1) | (pattern_lo & 0b1);
          auto c = palette[value] & 0x3F;
          set_pixel(x + tile_x, y + tile_y, PPU::SystemPalette[c]);
        }
      }
    }
  }
}

void NESDebugger::draw_palettes() {
  uint16_t horiz_off = 256 + 8;
  uint16_t vert_off = 128 + 48;

  for (int i = 0; i < 32; ++i) {
    auto c = ppu_read(0x3f00 + i);
    int x = (i & 0b1111) * 16 + horiz_off;
    int y = (i >> 4) * 16 + vert_off;
    for (int cx = 0; cx < 16; ++cx) {
      for (int cy = 0; cy < 16; ++cy) {
        set_pixel(x + cx, y + cy, PPU::SystemPalette[c]);
      }
    }
  }
}

void NESDebugger::render(RenderBuffer &buf) {
  std::fill(&frameBuffer[0][0], &frameBuffer[0][0] + frameBuffer.size() * 3,
            0xCF);

  draw_nametable();
  draw_sprites();
  draw_ptables();
  draw_palettes();

  // TODO(oren): draw a box around the nametable region currently in frame
  // draw_scroll_region();

  std::copy(frameBuffer.begin(), frameBuffer.end(), buf.begin());
}

void NESDebugger::set_pixel(int x, int y, std::array<uint8_t, 3> const &rgb) {
  // add a little space around the edge
  x += 16;
  y += 16;
  int pi = y * DBG_W + x;
  if (pi < frameBuffer.size()) {
    std::copy(std::begin(rgb), std::end(rgb), std::begin(frameBuffer[pi]));
  }
}

uint16_t NESDebugger::calc_nt_base(int x, int y) {
  if (y < 240) {
    if (x < 256) {
      return console_.ppu_registers_.BaseNTAddr[0];
    } else {
      return console_.ppu_registers_.BaseNTAddr[1];
    }
  } else {
    if (x < 256) {
      return console_.ppu_registers_.BaseNTAddr[2];
    } else {
      return console_.ppu_registers_.BaseNTAddr[3];
    }
  }
}

uint8_t NESDebugger::ppu_read(uint16_t addr) {
  // debug read (no address bus activity)
  return console_.mapper_->ppu_read(addr, true);
}
} // namespace sys
