#include "mappers/mapper_factory.hpp"
#include "mappers/axrom.hpp"
#include "mappers/base_mapper.hpp"
#include "mappers/cnrom.hpp"
#include "mappers/color_dreams.hpp"
#include "mappers/mmc1_2.hpp"
#include "mappers/mmc2.hpp"
#include "mappers/mmc3.hpp"
#include "mappers/nrom.hpp"
#include "mappers/uxrom.hpp"
#include "system.hpp"

namespace mapper {
std::unique_ptr<NESMapper>
MapperFactory(sys::NES &console, cart::Cartridge const &c, vid::Registers &reg,
              aud::Registers &areg, std::array<uint8_t, 0x100> &oam,
              ctrl::JoyPad &pad) {
  switch (c.mapper) {
  case 0:
    return std::make_unique<NROM>(console, c, reg, areg, oam, pad);
  case 1:
    return std::make_unique<MMC1>(console, c, reg, areg, oam, pad);
  case 2:
    return std::make_unique<UxROM>(console, c, reg, areg, oam, pad);
  case 3:
    return std::make_unique<CNROM>(console, c, reg, areg, oam, pad);
  case 4:
    return std::make_unique<MMC3>(console, c, reg, areg, oam, pad);
  case 7:
    return std::make_unique<AxROM>(console, c, reg, areg, oam, pad);
  case 9:
    return std::make_unique<MMC2>(console, c, reg, areg, oam, pad);
  case 11:
    return std::make_unique<ColorDreams>(console, c, reg, areg, oam, pad);
  default:
    throw std::runtime_error("Mapper not yet implemented...[" +
                             std::to_string(c.mapper) + "]");
  }
}
} // namespace mapper
