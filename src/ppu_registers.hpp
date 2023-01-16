#pragma once

#include <array>
#include <assert.h>
#include <cstdint>

// TODO(oren): Split this up. `Registers` is acting as both access point for PPU
// Registers *AND* a signalling medium between the CPU memory mapper and the
// PPU. If you squint, this is the same thing, but I'd like to tease some of
// that functionality apart. TBD.
namespace vid {
class Registers {
  static constexpr uint8_t BIT0 = 0b00000001;
  static constexpr uint8_t BIT1 = 0b00000010;
  static constexpr uint8_t BIT2 = 0b00000100;
  static constexpr uint8_t BIT3 = 0b00001000;
  static constexpr uint8_t BIT4 = 0b00010000;
  static constexpr uint8_t BIT5 = 0b00100000;
  static constexpr uint8_t BIT6 = 0b01000000;
  static constexpr uint8_t BIT7 = 0b10000000;

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

  void write(CName r, uint8_t val);
  uint8_t read(CName r);

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
  void setVBlankStarted();
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
    } else {
      V.XXXXX++;
    }
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
  bool handleNmi();

  void signalOamDma();
  uint16_t oamCycles();

private:
  struct scroll_var {
    uint8_t yyy : 3;
    uint8_t NN : 2;
    uint8_t YYYYY : 5;
    uint8_t XXXXX : 5;
  };

  //  https://forums.nesdev.org/viewtopic.php?t=21527
  scroll_var T;
  scroll_var V;
  uint8_t x;

  std::array<uint8_t, 8> regs_{};
  bool addr_latch_ = false;
  uint8_t io_latch_ = 0;
  uint16_t vram_addr_ = 0x0000;
  bool write_pending_ = false;
  bool read_pending_ = false;
  bool nmi_pending_ = false;
  uint16_t oam_cycles_ = 0;

  const static std::array<bool, 8> Writeable;
  const static std::array<bool, 8> Readable;
  const static std::array<uint16_t, 4> BaseNTAddr;
  const static std::array<uint8_t, 2> VRamAddrInc;
  const static std::array<uint16_t, 2> PTableAddr;
  const static std::array<uint8_t, 2> SpriteSize;
};

inline uint16_t Registers::baseNametableAddr() { return BaseNTAddr[V.NN]; }

inline uint8_t Registers::vRamAddrInc() {
  uint8_t i = (regs_[PPUCTRL] & BIT2) >> 2;
  return VRamAddrInc[i];
}

inline uint16_t Registers::spritePTableAddr(uint8_t idx) {
  uint8_t i = (spriteSize() == 8 ? (regs_[PPUCTRL] & BIT3) >> 3 : idx & 0b1);
  return PTableAddr[i];
}

inline uint16_t Registers::backgroundPTableAddr() {
  uint8_t i = (regs_[PPUCTRL] & BIT4) >> 4;
  return PTableAddr[i];
}

inline uint8_t Registers::spriteSize() {
  uint8_t i = (regs_[PPUCTRL] & BIT5) >> 5;
  return SpriteSize[i];
}

inline bool Registers::ppuMasterSlave() { return regs_[PPUCTRL] & BIT6; }

inline bool Registers::vBlankNMI() { return regs_[PPUCTRL] & BIT7; }

inline bool Registers::grayscale() { return regs_[PPUMASK] & BIT0; }
inline bool Registers::showBackgroundLeft8() { return regs_[PPUMASK] & BIT1; }
inline bool Registers::showSpritesLeft8() { return regs_[PPUMASK] & BIT2; }
inline bool Registers::showBackground() { return regs_[PPUMASK] & BIT3; }
inline bool Registers::showSprites() { return regs_[PPUMASK] & BIT4; }
inline bool Registers::emphasizeRed() { return regs_[PPUMASK] & BIT5; }
inline bool Registers::emphasizeGreen() { return regs_[PPUMASK] & BIT6; }
inline bool Registers::emphasizeBlue() { return regs_[PPUMASK] & BIT7; }

inline bool Registers::spriteOverflow() { return regs_[PPUSTATUS] & BIT5; }
inline bool Registers::spriteZeroHit() { return regs_[PPUSTATUS] & BIT6; }
inline void Registers::setSpriteZeroHit(bool val) {
  // if the new and current values differ, flip the appropriate bit
  regs_[PPUSTATUS] &= ~BIT6;
  if (val) {
    regs_[PPUSTATUS] |= BIT6;
  }
}
inline bool Registers::vBlankStarted() { return regs_[PPUSTATUS] & BIT7; }

inline void Registers::setVBlankStarted() { regs_[PPUSTATUS] |= BIT7; }
inline void Registers::clearVBlankStarted() { regs_[PPUSTATUS] &= ~BIT7; }

inline uint8_t Registers::oamAddr() { return regs_[OAMADDR]; }
inline uint8_t Registers::oamData() { return regs_[OAMDATA]; }

inline uint8_t Registers::scrollX() {
  return ((V.NN & 0b1) * 256) + (V.XXXXX * 8) + x;
}
inline uint8_t Registers::scrollY() {
  return (((V.NN >> 1) & 0b1) * 256) + (V.YYYYY * 8) + V.yyy;
}
inline uint16_t Registers::vRamAddr() { return vram_addr_; }

inline bool Registers::writePending() { return write_pending_; }
inline bool Registers::readPending() { return read_pending_; }

// TODO(oren): handle 514 (odd CPU cycle) case
inline void Registers::signalOamDma() { oam_cycles_ = 513 * 3; }

} // namespace vid
