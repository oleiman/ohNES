#include "system.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

using mapper::MapperFactory;

namespace sys {
NES::NES(std::string_view const &romfile, bool debug)
    : debug_(debug), cartridge_(romfile),
      mapper_(
          MapperFactory(*this, cartridge_, ppu_registers_, ppu_oam_, joypad_1)),
      ppu_(*mapper_, ppu_registers_, ppu_oam_), cpu_(*mapper_, false),
      debugger_(*this) {
  cpu_.registerTickHandler(std::bind(&NES::ppuTick, this));
  cpu_.registerTickHandler(std::bind(&NES::mapperTick, this));
  cpu_.reset();
  std::cerr << cartridge_ << std::endl;
}

void NES::step() {
  if (!debug_) {
    cpu_.step();
  } else if (!debugger_.paused()) {
    cpu_.debugStep(debugger_);
  }
}

bool NES::render(RenderBuffer &renderBuf) {

  // NOTE(oren): isFrameReady clears the frame ready flag regardless of status,
  // so effectively for each completed frame only one invocation will return
  // true until the next frame is completed.
  if (ppu_registers_.isFrameReady()) {
    std::copy(ppu_.frameBuffer().begin(), ppu_.frameBuffer().end(),
              renderBuf.begin());
    ppu_.clearFrame();
    return true;
  } else {
    return false;
  }
}

} // namespace sys
