#pragma once

#include "dbg/nes_debugger.hpp"
#include "util.hpp"

#include <array>
#include <assert.h>
#include <cstdint>
#include <iostream>

namespace mapper {
class NESMapper;
}

namespace sys {
class NESDebugger;
}

// TODO(oren): Split this up. `Registers` is acting as both access point for PPU
// Registers *AND* a signalling medium between the CPU memory mapper and the
// PPU. If you squint, this is the same thing, but I'd like to tease some of
// that functionality apart. TBD.
namespace vid {

class Registers {
  static constexpr int PPU_CLOCK_SPEED_HZ = 5369318;
  static constexpr int CY_600MS = PPU_CLOCK_SPEED_HZ / 1000 * 600;
  static constexpr uint16_t PPU_AMASK = 0x3FFF;
  static constexpr uint16_t PALETTE_BASE = 0x3F00;

public:
  enum CName {
    PPUCTRL = 0,
    PPUMASK,
    PPUSTATUS,
    OAMADDR,
    OAMDATA,
    PPUSCROLL,
    PPUADDR,
    PPUDATA,
  };
  // TODO(oren): consider OAMDMA transfers. might be tricky

  // Registers() { regs_[PPUCTRL] = 0x80; }

  void write(CName r, uint8_t val, mapper::NESMapper &);
  uint8_t read(CName r, mapper::NESMapper &);

  uint16_t cycle() const { return cycle_; }
  uint16_t scanline() const { return scanline_; }
  unsigned long frames() const { return frame_count_; }
  void nextFrame();
  bool isFrameReady();
  void tick();

  /*** PPUCTRL Accessors ***/
  uint16_t baseNametableAddr();
  uint8_t vRamAddrInc();
  uint16_t spritePTableAddr(uint8_t idx);
  uint16_t backgroundPTableAddr();
  uint8_t spriteSize();

  // false: read backdrop from EXT pins
  // true: output color on EXT pins
  bool ppuMasterSlave();
  bool vBlankNMI();
  bool isNmiSuppressed() { return suppress_vblank_; }

  // TODO(oren): scrollingMSB ??

  /*END PPUCTRL Accessors*/

  /*** PPUMASK Accessors ***/
  bool grayscale();
  bool showBackgroundLeft8();
  bool showSpritesLeft8();
  bool showBackground();
  bool showSprites();
  bool emphasizeRed();
  bool emphasizeGreen();
  bool emphasizeBlue();
  /*END PPUMASK Accessors*/

  /*** PPUSTATUS Accessors ***/
  bool spriteOverflow();
  bool spriteZeroHit();
  void setSpriteZeroHit(bool val);
  bool vBlankStarted();
  // Return false if vblank suppressed by PPUSTATUS special case
  bool setVBlankStarted();
  void clearVBlankStarted();
  /*END PPUSTATUS Accessors ***/

  /*** Address Accessors ***/
  uint8_t oamAddr();
  uint8_t oamData();
  uint16_t scrollAddr();
  uint8_t scrollX();
  uint8_t scrollY();
  uint8_t scrollX_fine();
  uint8_t scrollY_fine();
  uint8_t scrollX_coarse();
  uint8_t scrollY_coarse();
  void syncScroll() { V = T; }
  void syncScrollX() {
    V.XXXXX = T.XXXXX;
    V.NN = (V.NN & 0b10) | (T.NN & 0b01);
  }
  void syncScrollY() {
    V.YYYYY = T.YYYYY;
    V.yyy = T.yyy;
    V.NN = (V.NN & 0b01) | (T.NN & 0b10);
  }

  /*  Scroll Increment functions. Not useful right now since we generate the
   * whole bounding box the nametables at each pixel (ideally once per
   * scanline). Leaving these in place as a reference. */
  void incHorizScroll() {
    if (V.XXXXX == 31) {
      V.XXXXX = 0;
      V.NN ^= 0b01;
      // std::cout << "nt: " << std::bitset<2>(V.NN) << std::endl;
    } else {
      V.XXXXX++;
    }
    // std::cout << +V.XXXXX << std::endl;
  }
  void incVertScroll() {
    if (V.yyy < 7) {
      V.yyy++;
    } else {
      V.yyy = 0;
      if (V.YYYYY == 29) {
        V.YYYYY = 0;
        V.NN ^= 0b10;
      } else if (V.YYYYY == 31) {
        V.YYYYY = 0;
      } else {
        V.YYYYY++;
      }
    }
  }
  uint16_t vRamAddr();

  /*END Address Accessors ***/

  /*** Data Port ***/

  uint8_t getData();
  // place a byte in the PPU "internal buffer"
  void putData(uint8_t);
  bool writePending();
  bool readPending();
  void clearReadPending() { read_pending_ = false; }
  void clearWritePending() { write_pending_ = false; }
  bool handleNmi();

  void signalOamDma();
  uint16_t oamCycles();

private:
  uint16_t cycle_ = 0;
  uint16_t scanline_ = 261;
  unsigned long frame_count_ = 0;
  bool frame_ready_ = false;
  bool suppress_vblank_ = false;

  struct scroll_var {
    uint8_t yyy : 3;
    uint8_t NN : 2;
    uint8_t YYYYY : 5;
    uint8_t XXXXX : 5;

    uint16_t vram_addr() {
      uint16_t addr = ((static_cast<uint16_t>(yyy & 0b011) << 12) |
                       (static_cast<uint16_t>(NN) << 10) |
                       (static_cast<uint16_t>(YYYYY) << 5) |
                       static_cast<uint16_t>(XXXXX) | 0);
      // mask off the most significant bit
      return addr & PPU_AMASK;
    }

    void inc(uint8_t v) { vram_inc_ += v; }

  private:
    uint16_t vram_inc_ = 0;
  };

  //  https://forums.nesdev.org/viewtopic.php?t=21527
  scroll_var T;
  scroll_var V;
  uint8_t x;

  void incVRamAddr();

  std::array<uint8_t, 8> regs_{};
  bool write_toggle_ = false;
  struct {

    void write(uint8_t val) {
      refresh_.fill(cycle_);
      val_ = val;
    }

    uint8_t read(CName reg, uint8_t result = 0x00, bool isPalData = false) {
      switch (reg) {
      case PPUSTATUS:
        refresh_bits(result, 5, 8);
        break;
      case OAMDATA:
        refresh_bits(result, 0, 8);
      case PPUDATA:
        if (isPalData) {
          refresh_bits(result, 0, 6);
        } else {
          refresh_bits(result, 0, 8);
        }
        break;
      default:
        break;
      }
      return val_;
    }

    void tick() {
      ++cycle_;
      for (uint16_t i = 0; i < refresh_.size(); ++i) {
        // TODO(oren): could do this branchless
        if (cycle_ - refresh_[i] > CY_600MS) {
          // bit (i-1) decays to 0
          val_ &= (0xFF ^ (1 << i));
        }
      }
    }

  private:
    void refresh_bits(uint8_t in, uint8_t lo, uint8_t hi) {
      assert(lo < hi && hi <= 8);
      for (int i = lo; i < hi; ++i) {
        uint8_t in_bit = (in & (1 << i));
        if (in_bit) {
          val_ |= in_bit;
        } else {
          val_ &= (0xFF ^ (1 << i));
        }
        refresh_[i] = cycle_;
      }
    }
    uint8_t val_ = 0;
    std::array<unsigned long, 8> refresh_ = {};
    unsigned long cycle_ = 0;
  } io_latch_;

  uint16_t vram_addr_ = 0x0000;
  bool write_pending_ = false;
  uint8_t write_value_ = 0x00;
  bool read_pending_ = false;
  bool nmi_pending_ = false;
  uint16_t oam_cycles_ = 0;

  const static std::array<bool, 8> Writeable;
  const static std::array<bool, 8> Readable;
  const static std::array<uint16_t, 4> BaseNTAddr;
  const static std::array<uint8_t, 2> VRamAddrInc;
  const static std::array<uint16_t, 2> PTableAddr;
  const static std::array<uint8_t, 2> SpriteSize;

  friend class sys::NESDebugger;
};

inline uint16_t Registers::baseNametableAddr() { return BaseNTAddr[V.NN]; }

inline uint8_t Registers::vRamAddrInc() {
  uint8_t i = (regs_[PPUCTRL] & util::BIT2) >> 2;
  return VRamAddrInc[i];
}

inline uint16_t Registers::spritePTableAddr(uint8_t idx) {
  uint8_t i =
      (spriteSize() == 8 ? (regs_[PPUCTRL] & util::BIT3) >> 3 : idx & 0b1);
  return PTableAddr[i];
}

inline uint16_t Registers::backgroundPTableAddr() {
  uint8_t i = (regs_[PPUCTRL] & util::BIT4) >> 4;
  return PTableAddr[i];
}

inline uint8_t Registers::spriteSize() {
  uint8_t i = (regs_[PPUCTRL] & util::BIT5) >> 5;
  return SpriteSize[i];
}

inline bool Registers::ppuMasterSlave() { return regs_[PPUCTRL] & util::BIT6; }

inline bool Registers::vBlankNMI() { return regs_[PPUCTRL] & util::BIT7; }

inline bool Registers::grayscale() { return regs_[PPUMASK] & util::BIT0; }
inline bool Registers::showBackgroundLeft8() {
  return regs_[PPUMASK] & util::BIT1;
}
inline bool Registers::showSpritesLeft8() {
  return regs_[PPUMASK] & util::BIT2;
}
inline bool Registers::showBackground() { return regs_[PPUMASK] & util::BIT3; }
inline bool Registers::showSprites() { return regs_[PPUMASK] & util::BIT4; }
inline bool Registers::emphasizeRed() { return regs_[PPUMASK] & util::BIT5; }
inline bool Registers::emphasizeGreen() { return regs_[PPUMASK] & util::BIT6; }
inline bool Registers::emphasizeBlue() { return regs_[PPUMASK] & util::BIT7; }

inline bool Registers::spriteOverflow() {
  return regs_[PPUSTATUS] & util::BIT5;
}
inline bool Registers::spriteZeroHit() { return regs_[PPUSTATUS] & util::BIT6; }
inline void Registers::setSpriteZeroHit(bool val) {
  // if the new and current values differ, flip the appropriate bit
  regs_[PPUSTATUS] &= ~util::BIT6;
  if (val) {
    regs_[PPUSTATUS] |= util::BIT6;
  }
}
inline bool Registers::vBlankStarted() { return regs_[PPUSTATUS] & util::BIT7; }

inline uint8_t Registers::oamAddr() { return regs_[OAMADDR]; }
inline uint8_t Registers::oamData() { return regs_[OAMDATA]; }

inline uint8_t Registers::scrollX_fine() { return x; }
inline uint8_t Registers::scrollX_coarse() { return scrollX() & 0b11111000; };

inline uint8_t Registers::scrollX() {
  return ((V.NN & 0b1) * 256) + (V.XXXXX * 8) + x;
}

inline uint8_t Registers::scrollY_fine() { return scrollY() & 0b111; }
inline uint8_t Registers::scrollY() {
  return (((V.NN >> 1) & 0b1) * 256) + (V.YYYYY * 8) + V.yyy;
}
inline uint16_t Registers::vRamAddr() { return vram_addr_; }
inline void Registers::incVRamAddr() {
  vram_addr_ = (vram_addr_ + vRamAddrInc()) & PPU_AMASK;
}

inline bool Registers::writePending() { return write_pending_; }
inline bool Registers::readPending() { return read_pending_; }

// TODO(oren): handle 514 (odd CPU cycle) case
inline void Registers::signalOamDma() { oam_cycles_ = 513 * 3; }

} // namespace vid
