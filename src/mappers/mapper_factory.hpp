#pragma once

#include "apu_registers.hpp"
#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <memory>

namespace sys {
class NES;
}

namespace mapper {

std::unique_ptr<mapper::NESMapper>
MapperFactory(sys::NES &console, cart::Cartridge const &c, vid::Registers &reg,
              aud::Registers &areg, std::array<uint8_t, 0x100> &oam,
              ctrl::JoyPad &pad);
}
