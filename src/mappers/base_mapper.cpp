#include "mappers/base_mapper.hpp"

namespace mapper {
void BaseNESMapper::oamDma(AddressT base) {
  uint8_t oam_base = ppu_reg_.oamAddr();
  for (int i = 0; i < ppu_oam_.size(); ++i) {
    ppu_oam_[oam_base + i] = internal_[base | i];
  }
  ppu_reg_.signalOamDma();
}
} // namespace mapper
