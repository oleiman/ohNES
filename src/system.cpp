#include "system.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using mapper::MapperFactory;

namespace sys {
NES::NES(std::string_view const &romfile, bool debug)
    : debug_(debug), cartridge_(romfile),
      mapper_(
          MapperFactory(*this, cartridge_, ppu_registers_, ppu_oam_, joypad_1)),
      ppu_(*mapper_, ppu_registers_, ppu_oam_), cpu_(*mapper_, false),
      debugger_(true, false) {
  cpu_.reset();
  std::cerr << cartridge_ << std::endl;
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
}
} // namespace sys
