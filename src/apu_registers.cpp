#include "apu_registers.hpp"
#include "util.hpp"

namespace aud {

void Registers::write(CName r, uint8_t val, mapper::NESMapper &mapper) {
  assert(r != CName::_1 && r != CName::_2);

  if (r == CName::FRAME_CNT) {
    frame_control_reg_ = val;
    reset_ = true;
  } else if (r == CName::STATUS) {
    bool b4 = status_reg_ & util::BIT4;
    status_reg_ = val;
    bool af = status_reg_ & util::BIT4;
    if (!af || (!b4 && af)) {
      dmc_en_changed = true;
    }
  } else {
    generator_regs[static_cast<int>(r)] = val;
  }

  switch (r) {
  case P1_THI:
    p1_load_pending = true;
    p1_env_start = true;
    break;
  case P1_SWP:
    p1_sweep_reload = true;
    break;
  case P2_THI:
    p2_load_pending = true;
    p2_env_start = true;
    break;
  case P2_SWP:
    p2_sweep_reload = true;
    break;
  case TR_THI:
    tr_load_pending = true;
    tr_lin_load_pending = true;
    break;
  case NS_LCL:
    ns_load_pending = true;
    ns_env_start = true;
    break;
  case DMC_LOAD:
    dmc_direct_load = true;
    break;
  default:
    break;
  }
}

uint8_t Registers::read(CName r, mapper::NESMapper &) {
  if (r == STATUS) {
    clear_frame_interrupt_ = true;
    return fc_status_;
  }
  return 0;
}

double Registers::calc_duty_cycle(uint8_t val) const {
  uint8_t d = (val & (util::BIT7 | util::BIT6)) >> 6;

  switch (d & 0b11) {

  case 0:
    return 0.125;
  case 1:
    return 0.25;
  case 2:
    return 0.5;
  case 3:
    // TODO(oren): this should be a "negative polarity" quarter dc pulse
    // return 0.25;
    return 0.75;
  default:
    assert(false);
  }
}

} // namespace aud
