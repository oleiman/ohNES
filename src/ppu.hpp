#pragma once

#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <iostream>
#include <unordered_map>

namespace vid {
constexpr int WIDTH = 256;
constexpr int HEIGHT = 240;

void LoadSystemPalette(const std::string &fname);

class PPU {
  using AddressT = uint16_t;
  using DataT = uint8_t;
  using FrameBuffer = std::array<std::array<uint8_t, 3>, WIDTH * HEIGHT>;

  // TODO(oren): const correctness (requires some refactoring in
  // selectNametable)
  struct View {
    uint8_t x_min;
    uint8_t y_min;
    uint8_t x_max;
    uint8_t y_max;
    struct {
      int x;
      int y;
    } shift;
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

  enum class Priority {
    FG = 0,
    BG = 1,
  };

  // scroll is opposite mirroring.
  // e.g. horiz mirror (0) indicates vertical  scroll
  enum class Scroll {
    VERTICAL = 0,
    HORIZONTAL = 1,
    LOWER = 2,
    UPPER = 3,
  };

  union Sprite {
    struct {
      uint8_t xpos;
      uint8_t tile_lo;
      uint8_t tile_hi;
      union {
        struct {
          uint8_t palette_i : 2;
          uint8_t _ : 3;
          uint8_t priority : 1;
          uint8_t h_flip : 1;
          uint8_t v_flip : 1;
        } __attribute__((packed)) s;
        uint8_t v;
      } attrs;
      uint8_t idx;
      uint8_t _;
      uint16_t __;
    } __attribute__((packed)) s;
    uint64_t v;
  };

  friend std::ostream &operator<<(std::ostream &os, const Nametable &in);

public:
  PPU(mapper::NESMapper &mapper, Registers &registers,
      std::array<uint8_t, 256> &oam);

  FrameBuffer const &frameBuffer() { return framebuf_; }

  void step(uint16_t cycles, bool &nmi);

  // void reset();
  bool render();
  uint16_t currScanline() { return scanline_; }
  uint16_t currCycle() { return cycle_; }
  Nametable selectNametable(int x, int y);
  void renderBgPixel(uint16_t nt_base, View const &view, int abs_x, int abs_y);
  void renderSpritePixel(int abs_x, int abs_y);
  void evaluateSprites();

  // For debugging and ROM exploratory purposes
  void showTile(uint8_t x, uint8_t y, uint8_t bank, uint8_t tile);
  void showPatternTable();

  Scroll scrollType() { return Scroll(mapper_.mirroring()); }

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

  void SetSpriteZeroHit() {
    if (!szh_) {
      szh_ = true;
      registers_.setSpriteZeroHit(szh_);
    }
  }

  void ClearSpriteZeroHit() {
    if (szh_) {
      szh_ = false;
      registers_.setSpriteZeroHit(szh_);
    }
  }

  FrameBuffer framebuf_{};
  mapper::NESMapper &mapper_;
  Registers &registers_;
  std::array<uint8_t, 256> &oam_;
  uint16_t cycle_ = 0;
  uint16_t scanline_ = 261; // initialize to pre-render scanline
  std::array<uint8_t, 32> secondary_oam_{};
  bool bg_zero_ = false;
  bool szh_ = false;
  bool renderedSprite_ = false;

  std::array<Sprite, 8> sprites_{};

  static std::array<std::array<uint8_t, 3>, 64> SystemPalette;
  static std::unordered_map<Scroll, std::unordered_map<AddressT, AddressT>>
      SecondaryNTMap;

  friend void LoadSystemPalette(const std::string &fname);
};

} // namespace vid
