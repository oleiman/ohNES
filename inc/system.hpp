#pragma once

#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "mappers/mapper_factory.hpp"
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

private:
  bool debug_;
  cart::Cartridge cartridge_;
  vid::Registers ppu_registers_;
  std::array<uint8_t, 256> ppu_oam_ = {};
  std::unique_ptr<mapper::NESMapper> cpu_map_;
  vid::PPU ppu_;
  cpu::M6502 cpu_;
  dbg::Debugger debugger_;
};
} // namespace sys
