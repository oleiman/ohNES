#pragma once

#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "ppu_registers.hpp"

#include <array>
#include <memory>

namespace mapper {

std::unique_ptr<mapper::BaseNESMapper>
MapperFactory(cart::Cartridge const &c, vid::Registers &reg,
              std::array<mapper::BaseNESMapper::DataT, 0x100> &oam,
              ctrl::JoyPad &pad);
}
