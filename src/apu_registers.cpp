#include "apu_registers.hpp"
#include "util.hpp"

namespace aud {

void Registers::write(CName r, uint8_t val, mapper::NESMapper &mapper) {
  assert(r != CName::_1 && r != CName::_2);

  if (r == CName::FRAME_CNT) {
    frame_control_reg_ = val;
    // seq_mode_ = (val & util::BIT7) >> 7;
    // inhibit_irq_ = (val & util::BIT6);
    // TODO(oren): this effect is delayed by 3 or 4 cycles depending on when the
    // write occurred.
    reset_ = true;
  } else if (r == CName::STATUS) {
    status_reg_ = val;
  } else {
    generator_regs[static_cast<int>(r)] = val;
  }

  if (r == P1_THI) {
    p1_load_pending = true;
    p1_env_start = true;
  } else if (r == P1_SWP) {
    p1_sweep_reload = true;
  } else if (r == P2_THI) {
    p2_load_pending = true;
    p2_env_start = true;
  } else if (r == P2_SWP) {
    p2_sweep_reload = true;
  } else if (r == TR_THI) {
    tr_load_pending = true;
    tr_lin_load_pending = true;
  } else if (r == NS_LCL) {
    ns_load_pending = true;
    ns_env_start = true;
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
