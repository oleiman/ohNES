#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <memory>

namespace mapper {

std::unique_ptr<mapper::NESMapper>
MapperFactory(cart::Cartridge const &c, vid::Registers &reg,
              std::array<uint8_t, 0x100> &oam, ctrl::JoyPad &pad);
}
