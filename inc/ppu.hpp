#pragma once

#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <iostream>

namespace vid {
constexpr int WIDTH = 256;
constexpr int HEIGHT = 240;

class PPU {
  using AddressT = uint16_t;
  using DataT = uint8_t;
  using FrameBuffer = std::array<std::array<uint8_t, 3>, WIDTH * HEIGHT>;

  // TODO(oren): const correctness (requires some refactoring in
  // selectNametable)
  struct View {
    struct Shift {
      int x;
      int y;
    };
    uint8_t x_min;
    uint8_t y_min;
    uint8_t x_max;
    uint8_t y_max;
    Shift shift;
    bool contains(int x, int y) const {
      x -= shift.x;
      y -= shift.y;
      return (x >= x_min && x <= x_max && y >= y_min && y <= y_max);
    }
  };

  struct Nametable {
    AddressT base;
    View view;
    bool describesPixel(int x, int y) const { return view.contains(x, y); }
  };

  friend std::ostream &operator<<(std::ostream &os, const Nametable &in);

public:
  PPU(mapper::NESMapper &mapper, Registers &registers,
      std::array<uint8_t, 256> &oam);

  FrameBuffer const &frameBuffer() { return framebuf_; }

  void step(uint16_t cycles, bool &nmi);
  // void reset();
  bool render();
  Nametable selectNametable(int x, int y);
  void renderBgPixel(uint16_t nt_base, View const &view, int abs_x, int abs_y);
  void renderSpritePixel(int abs_x, int abs_y);
  void evaluateSprites();

  // For debugging and ROM exploratory purposes
  void showTile(uint8_t x, uint8_t y, uint8_t bank, uint8_t tile);
  void showPatternTable();

private:
  void visibleLine();
  void vBlankLine();
  void set_pixel(uint8_t x, uint8_t y, std::array<uint8_t, 3> const &rgb);
  std::array<uint8_t, 4> bgPalette(uint16_t base, uint16_t tile_x,
                                   uint16_t tile_y);
  std::array<uint8_t, 4> spritePalette(uint8_t pidx);
  void tick();

  // TODO(oren): find a way to advance the clock from in here
  DataT readByte(AddressT addr) { return mapper_.ppu_read(addr); }
  void writeByte(AddressT addr, DataT data) { mapper_.ppu_write(addr, data); }

  std::pair<Nametable, Nametable> constructNametables();

  FrameBuffer framebuf_{};
  mapper::NESMapper &mapper_;
  Registers &registers_;
  std::array<uint8_t, 256> &oam_;
  uint16_t cycle_ = 0;
  uint16_t scanline_ = 261; // initialize to pre-render scanline
  std::array<uint8_t, 32> secondary_oam_{};
  bool bg_nonzero_ = false;
  bool sprite_nonzero_ = false;

  std::array<uint8_t, 8> sprite_attrs{};
  std::array<uint8_t, 8> sprite_xpos{};
  std::array<uint8_t, 8> sprite_tiles_l{};
  std::array<uint8_t, 8> sprite_tiles_h{};

  std::array<std::array<uint8_t, 3>, 64> SystemPalette = {
      {
          {0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0},
          {0x44, 0x00, 0x96}, {0xA1, 0x00, 0x5E}, {0xC7, 0x00, 0x28},
          {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00}, {0x5C, 0x2F, 0x00},
          {0x10, 0x45, 0x00}, {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E},
          {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05}, //
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
          {0x11, 0x11, 0x11},
      },
  };
};

} // namespace vid
