#include "cartridge.hpp"
#include "joypad.hpp"
#include "mappers/base_mapper.hpp"
#include "memory.hpp"
#include "ppu_registers.hpp"

#include <array>

namespace sys {
class NES;
}

namespace mapper {

class MMC3 : public NESMapperBase<MMC3> {
public:
  MMC3(sys::NES &console, cart::Cartridge const &c, vid::Registers &reg,
       std::array<DataT, 0x100> &oam, ctrl::JoyPad &pad)
      : NESMapperBase<MMC3>(console, c, reg, oam, pad),
        mirroring_(cart_.mirroring ^ 0b1) {}

  void cartWrite(AddressT addr, DataT data);
  DataT cartRead(AddressT addr);
  void chrWrite(AddressT addr, DataT data);
  DataT chrRead(AddressT addr);

  uint8_t mirroring() const override {
    // NOTE(oren): for this mapper, 0 means vertical, 1 means horizontal
    // rest of emu expects the opposite, so we flip the mirroring bit
    return mirroring_ ^ 0b1;
  }

  bool setPpuABus(AddressT) override;

private:
  uint8_t mirroring_ = 0;
  uint8_t prgRamProtect_ = 0;
  uint8_t irqLatchVal_ = 0;
  AddressT ppuABus_ = 0;

  std::array<uint32_t, 8> bankConfig_ = {};

  uint8_t prgMapMode() const { return (prgBankSelect >> 6) & 0b1; }
  uint8_t chrMapMode() const { return (prgBankSelect >> 7) & 0b1; }
  void tick(uint16_t c) override;

  struct {
    uint8_t val = 0;
    bool rld = true;
    void clear() {
      val = 0;
      rld = true;
    }

    bool reload(uint8_t v) {
      if (rld || val == 0) {
        val = v;
        rld = false;
        return true;
      }
      return false;
    }
  } irqCounter_;

  bool irqEnabled_ = false;
  bool pending_irq_ = false;

  enum class A12_STATE {
    LOW,
    HIGH,
  };

  struct {
    A12_STATE state = A12_STATE::LOW;
    unsigned long long timer = 0;
  } a12_state_ = {};
  void irqEnable(bool e);

  // TODO(oren): may want to remove for mmc6 compatibility
  bool prgRamEnabled() const {
    return cart_.hasPrgRam && prgRamProtect_ & 0b10000000;
  }
  bool prgRamWriteProtect() const { return prgRamProtect_ & 0b01000000; }

  bool genIrq();
};
} // namespace mapper
