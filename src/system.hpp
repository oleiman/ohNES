#pragma once

#include "cartridge.hpp"
#include "cpu.hpp"
#include "mappers/mapper_factory.hpp"
#include "nes_debugger.hpp"
#include "ppu.hpp"

#include <string>

namespace sys {

class NES {
  using RenderBuffer =
      std::array<std::array<uint8_t, 3>, vid::WIDTH * vid::HEIGHT>;

public:
  NES(std::string_view const &romfile, bool debug = false);
  ~NES() = default;
  void step();
  bool render(RenderBuffer &renderBuf);
  void reset() { cpu_.reset(); };
  cpu::CpuState const &state() { return cpu_.state(); }
  ctrl::JoyPad joypad_1;
  bool &irqPin() { return cpu_.irqPin(); }
  uint16_t currScanline() { return ppu_.currScanline(); }
  uint16_t currPpuCycle() { return ppu_.currCycle(); }

  NESDebugger &debugger() { return debugger_; }

private:
  void ppuTick() { ppu_.step(3, cpu_.nmiPin()); }
  void mapperTick() { mapper_->tick(1); }

  bool debug_;
  cart::Cartridge cartridge_;
  vid::Registers ppu_registers_;
  std::array<uint8_t, 256> ppu_oam_ = {};
  std::unique_ptr<mapper::NESMapper> mapper_;
  vid::PPU ppu_;
  cpu::M6502 cpu_;
  NESDebugger debugger_;

  friend class NESDebugger;
};
} // namespace sys
