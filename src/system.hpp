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
  bool &irqPin() { return cpu_.irqPin(); }
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
  void ppuTick() { ppu_.step(3 + ppu_registers_.oamCycles(), cpu_.nmiPin()); }
  void mapperTick() { mapper_->tick(1); }
  void apuTick() { apu_.step(cpu_.irqPin()); }

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
