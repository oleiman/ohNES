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
      std::array<uint8_t, 256> &oam)
      : mapper_(mapper), registers_(registers), oam_(oam) {}

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

  const static std::array<std::array<uint8_t, 3>, 64> SystemPalette;
};

} // namespace vid
