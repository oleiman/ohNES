#pragma once

#include "apu.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "dbg/nes_debugger.hpp"
#include "mappers/mapper_factory.hpp"
#include "ppu.hpp"

#include <string>

namespace sys {

class NES {
  using RenderBuffer =
      std::array<std::array<uint8_t, 3>, vid::WIDTH * vid::HEIGHT>;

public:
  NES(std::string_view const &romfile, bool debug = false, bool quiet = false);
  ~NES() = default;
  void step();
  bool render(RenderBuffer &renderBuf);
  void reset(bool force = false) {
    cpu_.reset(force);
    apu_.reset(force);
  };
  void reset(uint16_t addr) { cpu_.reset(static_cast<uint16_t>(addr)); }
  cpu::CpuState const &state() { return cpu_.state(); }
  uint16_t currScanline() { return ppu_.currScanline(); }
  uint16_t currPpuCycle() { return ppu_.currCycle(); }

  const cart::Cartridge &cart() const { return cartridge_; }

  NESDebugger &debugger() { return debugger_; }
  mapper::NESMapper &mapper() { return *mapper_; }

  bool paused() const { return debug_ && debugger_.paused(); }

  // advance the frame in headless mode (primarily for testing purposes)
  bool checkFrame() { return ppu_registers_.isFrameReady(); }

  bool debug_;
  ctrl::JoyPad joypad_1;

private:
  bool ppuTick() {
    ppu_.step(1 + ppu_registers_.oamCycles(), cpu_.nmiPin());
    // CPU should poll the nmi line at the beginning of the second "half" of the
    // cycle. we can't subdivide a cpu clock any further, so we'll poll after
    // the first PPU cycle clocked by each CPU tick.
    cpu_.pollNmi();
    ppu_.step(1, cpu_.nmiPin());
    ppu_.step(1, cpu_.nmiPin());
    return false;
  }
  bool mapperTick() {
    mapper_->tick(1);
    return mapper_->pendingIrq();
  }
  bool apuTick() {
    apu_.step();
    if (apu_.stallCpu()) {
      // TODO(oren): determine stall length cleverly
      cpu_.stall(4);
      // for (int i = 0; i < 4; ++i) {
      //   apu_.step(cpu_.irqPin());
      // }
    }
    return apu_.pendingIrq();
  }

  cart::Cartridge cartridge_;
  vid::Registers ppu_registers_;
  aud::Registers apu_registers_;
  std::array<uint8_t, 256> ppu_oam_ = {};
  std::unique_ptr<mapper::NESMapper> mapper_;
  vid::PPU ppu_;
  aud::APU apu_;
  cpu::M6502 cpu_;
  NESDebugger debugger_;

  friend class NESDebugger;
};
} // namespace sys
