#include "system.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using mapper::MapperFactory;

namespace sys {
NES::NES(std::string_view const &romfile, bool debug)
    : debug_(debug), cartridge_(romfile), ppu_map_(cartridge_), ppu_(ppu_map_),
      cpu_map_(MapperFactory(cartridge_, ppu_.registers, ppu_.oam, joypad_1)),
      cpu_(*cpu_map_, false), debugger_(true, false) {
  cpu_.reset();
}

void NES::step() {
  auto c = (debug_ ? cpu_.debugStep(debugger_) : cpu_.step());
  ppu_.step(c * 3, cpu_.nmiPin());
}

bool NES::render(RenderBuffer &renderBuf) {
  if (cpu_.nmiPin() && ppu_.render()) {
    std::copy(ppu_.frameBuffer().begin(), ppu_.frameBuffer().end(),
              renderBuf.begin());
    return true;
  } else {
    return false;
  }
} // namespace sys

} // namespace sys
