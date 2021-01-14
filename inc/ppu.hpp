#pragma once

#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <iostream>

namespace vid {
const int WIDTH = 256;
const int HEIGHT = 240;

class PPU {
  using AddressT = uint16_t;
  using DataT = uint8_t;
  using FrameBuffer = std::array<std::array<uint8_t, 3>, WIDTH * HEIGHT>;

public:
  Registers registers;
  std::array<uint8_t, 256> oam = {};

  PPU(mem::Mapper &mapper) : mapper_(mapper) {}

  FrameBuffer &frameBuffer() { return framebuf_; }

  void step(uint16_t cycles, bool &nmi);
  void renderSprites();
  void renderBackground();

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
  DataT readByte(AddressT addr) { return mapper_.read(addr); }
  void writeByte(AddressT addr, DataT data) { mapper_.write(addr, data); }

  FrameBuffer framebuf_{};
  mem::Mapper &mapper_;
  uint16_t cycle_ = 0;
  uint16_t scanline_ = 261; // initialize to pre-render scanline
  std::array<uint8_t, 32> oam_2_{};

  const static std::array<std::array<uint8_t, 3>, 64> SystemPalette;
};

} // namespace vid
