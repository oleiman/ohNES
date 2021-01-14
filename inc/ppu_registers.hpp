#pragma once

#include <array>
#include <cstdint>

namespace vid {
class Registers {
  static const uint8_t BIT0 = 0b00000001;
  static const uint8_t BIT1 = 0b00000010;
  static const uint8_t BIT2 = 0b00000100;
  static const uint8_t BIT3 = 0b00001000;
  static const uint8_t BIT4 = 0b00010000;
  static const uint8_t BIT5 = 0b00100000;
  static const uint8_t BIT6 = 0b01000000;
  static const uint8_t BIT7 = 0b10000000;

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
  uint16_t spritePTableAddr();
  uint16_t backgroundPTableAddr();
  std::array<uint8_t, 2> const &spriteSize();

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
  // TODO(oren): stores 5 lsb previously written into a PPU
  // not sure whether we need to do anything with these
  // In general, some work is required here as the bits of the
  // status register appear to be set by the PPU in response to
  // certain events, e.g. VBlank start/end
  bool spriteOverflow();
  bool spriteZeroHit();
  bool vBlankStarted();
  void setVBlankStarted();
  void clearVBlankStarted();
  /*END PPUSTATUS Accessors ***/

  /*** Address Accessors ***/
  uint8_t oamAddr();
  uint8_t oamData();
  uint16_t scrollAddr();
  uint16_t vRamAddr();
  /*END Address Accessors ***/

  /*** Data Port ***/
  // NOTE(oren): the way I'm thinking about this, the read/write interface
  // is for the CPU only, whereas these accessors are for the PPU
  // With video ram increment happening only on the PPU accesses
  // on the other hand, PPU needs to know somehow whether the CPU is reading
  // or writing, when to flush its buffer, etc

  uint8_t getData();
  // this is like filling the internal buffer
  void putData(uint8_t);
  bool writePending();
  bool readPending();
  bool handleNmi();

  void signalOamDma();
  uint16_t oamCycles();
  std::array<uint8_t, 256> &oam() { return oam_; }

private:
  std::array<uint8_t, 8> regs_{};
  bool addr_latch_ = false;
  uint16_t scroll_addr_ = 0x0000;
  uint16_t vram_addr_ = 0x0000;
  bool write_pending_ = false;
  bool read_pending_ = false;
  bool nmi_pending_ = false;
  uint16_t oam_cycles_ = 0;
  std::array<uint8_t, 256> oam_{};

  const static std::array<bool, 8> Writeable;
  const static std::array<bool, 8> Readable;
  const static std::array<uint16_t, 4> BaseNTAddr;
  const static std::array<uint8_t, 2> VRamAddrInc;
  const static std::array<uint16_t, 2> PTableAddr;
  const static std::array<std::array<uint8_t, 2>, 2> SpriteSize;
};

inline uint16_t Registers::baseNametableAddr() {
  return BaseNTAddr[regs_[PPUCTRL] & (BIT0 | BIT1)];
}

inline uint8_t Registers::vRamAddrInc() {
  uint8_t i = (regs_[PPUCTRL] & BIT2) >> 2;
  return VRamAddrInc[i];
}

inline uint16_t Registers::spritePTableAddr() {
  uint8_t i = (regs_[PPUCTRL] & BIT3) >> 3;
  return PTableAddr[i];
}

inline uint16_t Registers::backgroundPTableAddr() {
  uint8_t i = (regs_[PPUCTRL] & BIT4) >> 4;
  return PTableAddr[i];
}

inline std::array<uint8_t, 2> const &Registers::spriteSize() {
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
inline bool Registers::vBlankStarted() { return regs_[PPUSTATUS] & BIT7; }

inline void Registers::setVBlankStarted() { regs_[PPUSTATUS] |= BIT7; }
inline void Registers::clearVBlankStarted() { regs_[PPUSTATUS] &= ~BIT7; }

inline uint8_t Registers::oamAddr() { return regs_[OAMADDR]; }
inline uint8_t Registers::oamData() { return regs_[OAMDATA]; }

// TODO(oren): logic for reading between the two prescribed register writes?
// again, this should probably go inside the PPU
inline uint16_t Registers::scrollAddr() { return scroll_addr_; }
inline uint16_t Registers::vRamAddr() { return vram_addr_; }

inline uint8_t Registers::getData() {
  write_pending_ = false;
  vram_addr_ += vRamAddrInc();
  return regs_[PPUDATA];
}

inline void Registers::putData(uint8_t data) {
  read_pending_ = false;
  vram_addr_ += vRamAddrInc();
  regs_[PPUDATA] = data;
}
inline bool Registers::writePending() { return write_pending_; }
inline bool Registers::readPending() { return read_pending_; }
inline bool Registers::handleNmi() {
  auto tmp = nmi_pending_;
  nmi_pending_ = false;
  return tmp;
}

// TODO(oren): handle 514 (odd CPU cycle) case
inline void Registers::signalOamDma() { oam_cycles_ = 513 * 3; }
inline uint16_t Registers::oamCycles() {
  auto v = oam_cycles_;
  oam_cycles_ = 0;
  return v;
}

} // namespace vid
