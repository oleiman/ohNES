#include "apu_registers.hpp"
#include "util.hpp"

namespace aud {

void Registers::write(CName r, uint8_t val, mapper::NESMapper &mapper) {
  assert(r != CName::_1 && r != CName::_2);

  if (r == CName::FRAME_CNT) {
    frame_control_reg_ = val;
    fc_reset_ = true;
  } else if (r == CName::STATUS) {
    status_reg_ = val;
    dmc_en_changed = true;
    // bool b4 = status_reg_ & util::BIT4;
    // status_reg_ = val;
    // bool af = status_reg_ & util::BIT4;
    // if (!af || (!b4 && af)) {
    //   dmc_en_changed = true;
    // }
  } else {
    generator_regs[static_cast<int>(r)] = val;
  }

  switch (r) {
  case P1_THI:
    set_channel_flag(LcLoadFlags, ChannelId::PULSE_1);
    set_channel_flag(EnvStartFlags, ChannelId::PULSE_1);
    break;
  case P1_SWP:
    set_channel_flag(SweepReloadFlags, ChannelId::PULSE_1);
    break;
  case P2_THI:
    set_channel_flag(LcLoadFlags, ChannelId::PULSE_2);
    set_channel_flag(EnvStartFlags, ChannelId::PULSE_2);
    break;
  case P2_SWP:
    set_channel_flag(SweepReloadFlags, ChannelId::PULSE_2);
    break;
  case TR_THI:
    set_channel_flag(LcLoadFlags, ChannelId::TRIANGLE);
    tr_lin_load_pending = true;
    break;
  case NS_LCL:
    set_channel_flag(LcLoadFlags, ChannelId::NOISE);
    set_channel_flag(EnvStartFlags, ChannelId::NOISE);
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

Registers::ChannelFlags Registers::EnvStartFlags = {
    false, false, false, false, false,
};

Registers::ChannelFlags Registers::LcLoadFlags = {
    true, true, true, true, false,
};

Registers::ChannelFlags Registers::SweepReloadFlags = {
    false, false, false, false, false,
};

} // namespace aud
