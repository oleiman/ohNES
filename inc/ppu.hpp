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
  PPU(mem::Mapper &mapper, std::function<void(void)> nmi, Registers &reg)
      : mapper_(mapper), nmi_(nmi), reg_(reg) {}

  FrameBuffer &frameBuffer() { return framebuf_; }

  void step(uint8_t cycles);
  void tick();

  void renderBackground() {
    auto bank = reg_.backgroundPTableAddr();
    auto nt_base = reg_.baseNametableAddr();
    std::cout << std::hex << +nt_base << " " << +bank << std::dec << std::endl;

    for (int i = 0; i < 0x03c0; ++i) {
      auto tile = static_cast<uint16_t>(readByte(nt_base + i));
      auto tile_x = i % 32;
      auto tile_y = i / 32;
      auto palette = bgPalette(nt_base, tile_x, tile_y);
      auto tile_base = bank + tile * 16;
      for (int y = 0; y < 8; ++y) {
        auto upper = readByte(tile_base + y);
        auto lower = readByte(tile_base + y + 8);
        for (int x = 7; x >= 0; x--) {
          auto value = ((upper & 1) << 1) | (lower & 1);
          upper >>= 1;
          lower >>= 1;
          uint8_t rgb = palette[value];
          set_pixel(tile_x * 8 + x, tile_y * 8 + y, SystemPalette[rgb]);
        }
      }
    }
  }

  void showPatternTable() {
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

  void showTile(uint8_t x, uint8_t y, uint8_t bank, uint8_t tile) {
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

private:
  void serviceMemRequest();

  void visibleLine();
  void vBlankLine();
  void set_pixel(uint8_t x, uint8_t y, std::array<uint8_t, 3> const &rgb) {
    int pi = y * WIDTH + x;
    for (int i = 0; i < 3; ++i) {
      framebuf_[pi][i] = rgb[i];
    }
  }

  std::array<uint8_t, 4> bgPalette(uint16_t base, uint16_t tile_x,
                                   uint16_t tile_y);

  DataT readByte(AddressT addr) { return mapper_.read(addr); }
  void writeByte(AddressT addr, DataT data) { mapper_.write(addr, data); }

  FrameBuffer framebuf_{};
  mem::Mapper &mapper_;
  std::function<void(void)> nmi_;
  Registers &reg_;
  uint16_t cycle_ = 0;
  uint16_t scanline_ = 261; // initialize to pre-render scanline, i guess
  std::array<uint8_t, 32> oam_2_;

  bool mem_in_progress = false;
  uint8_t step_c_;

  const static std::array<std::array<uint8_t, 3>, 64> SystemPalette;
};

} // namespace vid
