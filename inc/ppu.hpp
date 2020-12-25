#pragma once

#include "memory.hpp"

#include <array>
#include <iostream>

namespace ppu {
const int WIDTH = 256;
const int HEIGHT = 240;

class Registers {

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

private:
  std::array<uint8_t, 8> regs_{};
  bool addr_latch_ = false;
  uint16_t scroll_addr_ = 0x0000;
  uint16_t vram_addr_ = 0x0000;
  bool write_pending_ = false;
  bool read_pending_ = false;

  const static std::array<bool, 8> Writeable;
  const static std::array<bool, 8> Readable;
  const static std::array<uint16_t, 4> BaseNTAddr;
  const static std::array<uint8_t, 2> VRamAddrInc;
  const static std::array<uint16_t, 2> PTableAddr;
  const static std::array<std::array<uint8_t, 2>, 2> SpriteSize;
};

class PPU {
  using AddressT = uint16_t;
  using DataT = uint8_t;
  using FrameBuffer = std::array<std::array<uint8_t, 3>, WIDTH * HEIGHT>;

public:
  PPU(mem::Bus<AddressT> &abus, mem::Bus<DataT> &mbus,
      std::function<void(void)> nmi)
      : address_bus_(abus), data_bus_(mbus), nmi_(nmi) {}

  FrameBuffer &frameBuffer() { return framebuf_; }

  // TODO(oren): remove
  void initFramebuf() {
    for (int j = 0; j < HEIGHT; ++j) {
      for (int i = 0; i < WIDTH; ++i) {
        set_pixel(i, j, SystemPalette[1]);
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
          set_pixel(xi, yi, SystemPalette[0x0D]);
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
  void set_pixel(uint8_t x, uint8_t y, std::array<uint8_t, 3> const &rgb) {
    int pi = y * WIDTH + x;
    for (int i = 0; i < 3; ++i) {
      framebuf_[pi][i] = rgb[i];
    }
  }

  DataT readByte(AddressT addr) {
    address_bus_.put(addr);
    return data_bus_.get();
  }

  FrameBuffer framebuf_{};
  mem::Bus<AddressT> &address_bus_;
  mem::Bus<DataT> &data_bus_;
  std::function<void(void)> nmi_;
  const static std::array<std::array<uint8_t, 3>, 64> SystemPalette;
};
} // namespace ppu
