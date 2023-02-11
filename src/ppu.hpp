#pragma once

#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <iostream>
#include <unordered_map>

namespace sys {
class NESDebugger;
}

namespace vid {
constexpr int WIDTH = 256;
constexpr int HEIGHT = 240;

void LoadSystemPalette(const std::string &fname);

class PPU {
  using AddressT = uint16_t;
  using DataT = uint8_t;
  using FrameBuffer = std::array<std::array<uint8_t, 3>, WIDTH * HEIGHT>;

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

public:
  PPU(mapper::NESMapper &mapper, Registers &registers,
      std::array<uint8_t, 256> &oam);

  FrameBuffer const &frameBuffer() { return framebuf_; }

  void step(uint16_t cycles, bool &nmi);

  bool rendering();
  uint16_t currScanline() { return scanline_; }
  uint16_t currCycle() { return cycle_; }

  // For debugging and ROM exploratory purposes
  void showTile(uint8_t x, uint8_t y, uint8_t bank, uint8_t tile);
  void showPatternTable();

private:
  void visibleLine(bool pre_render = false);
  void handleBackground(bool pre_render);
  void handleSprites(bool pre_render);

  void vBlankLine();
  void set_pixel(uint8_t x, uint8_t y, std::array<uint8_t, 3> const &rgb);
  std::array<uint8_t, 4> bgPalette();
  std::array<uint8_t, 4> spritePalette(uint8_t pidx);

  void renderBgPixel(int abs_x, int abs_y);
  void renderSpritePixel(int abs_x, int abs_y);
  void clearOam();
  void evaluateSprites();
  void fetchSprites_old();
  void fetchSprites();

  Scroll scrollType() { return Scroll(mapper_.mirroring()); }

  enum class PTOff {
    LOW = 0,
    HIGH = 8,
  };

  void LoadBackground();
  void fetchNametable();
  void fetchAttribute();
  void fetchPattern(PTOff plane);

  template <typename V, typename L> struct ShiftReg {
    V value;
    L latch;

    void Load() {
      // std::cout << "b: " << std::bitset<16>(value) << std::endl;
      value |= (static_cast<V>(latch) << ((sizeof(V) - sizeof(L)) * 8));
      // std::cout << "a: " << std::bitset<16>(value) << std::endl;
    }
    void Shift() { value >>= 1; }
    void Latch(L l) { latch = l; }
  };

  struct {
    void Load() {
      pattern_lo.Load();
      pattern_hi.Load();
      attr_lo.Load();
      attr_hi.Load();
    }

    void Shift() {
      pattern_lo.Shift();
      pattern_hi.Shift();
      attr_lo.Shift();
      attr_hi.Shift();
    }

    ShiftReg<uint16_t, uint8_t> pattern_lo;
    ShiftReg<uint16_t, uint8_t> pattern_hi;
    ShiftReg<uint16_t, uint8_t> attr_lo;
    ShiftReg<uint16_t, uint8_t> attr_hi;
    uint8_t shift_counter = 0;
  } backgroundSR_;

  uint8_t nametable_reg;

  void tick();

  // TODO(oren): find a way to advance the clock from in here
  DataT readByte(AddressT addr) { return mapper_.ppu_read(addr); }
  void writeByte(AddressT addr, DataT data) { mapper_.ppu_write(addr, data); }

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
  unsigned long long frame_count_ = 0;
  std::array<uint8_t, 32> secondary_oam_{};
  bool bg_zero_ = false;
  bool szh_ = false;

  std::array<Sprite, 8> sprites_{};
  std::array<Sprite, 8> sprites_staging_{};
  Sprite dummy_sprite_ = {.v = 0};
  uint8_t oam_n_ = 0;
  uint8_t oam_m_ = 0;
  uint8_t sec_oam_n_ = 0;
  bool sec_oam_write_enable_ = false;

  friend void LoadSystemPalette(const std::string &fname);
  friend class sys::NESDebugger;

public:
  static std::array<std::array<uint8_t, 3>, 64> SystemPalette;
};

} // namespace vid
