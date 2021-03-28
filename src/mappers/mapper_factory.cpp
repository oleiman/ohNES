#include "mappers/mapper_factory.hpp"
#include "mappers/base_mapper.hpp"
#include "mappers/cnrom.hpp"
#include "mappers/mmc1.hpp"
#include "mappers/nrom.hpp"
#include "mappers/uxrom.hpp"

namespace mapper {
std::unique_ptr<NESMapper> MapperFactory(cart::Cartridge const &c,
                                         vid::Registers &reg,
                                         std::array<uint8_t, 0x100> &oam,
                                         ctrl::JoyPad &pad) {
  switch (c.mapper) {
  case 0:
    return std::make_unique<NROM>(c, reg, oam, pad);
  case 1:
    return std::make_unique<MMC1>(c, reg, oam, pad);
  case 2:
    return std::make_unique<UxROM>(c, reg, oam, pad);
  case 3:
    return std::make_unique<CNROM>(c, reg, oam, pad);
  default:
    throw std::runtime_error("Mapper not yet implemented...[" +
                             std::to_string(c.mapper) + "]");
  }
}
} // namespace mapper
