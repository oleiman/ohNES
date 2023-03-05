#pragma once

#include "dbg/nes_debugger.hpp"
#include "util.hpp"

namespace aud {

class Registers {
public:
  enum CName {
    P1_CTRL = 0x00,
    P1_SWP = 0x01,
    P1_TLO = 0x02,
    P1_THI = 0x03,
    P2_CTRL = 0x04,
    P2_SWP = 0x05,
    P2_TLO = 0x06,
    P2_THI = 0x07,
    TR_CTRL = 0x08,
    TR__ = 0x09,
    TR_TLO = 0x0A,
    TR_THI = 0x0B,
    NS_CTRL = 0x0C,
    NS__ = 0x0D,
    NS_LNP = 0x0E,
    NS_LCL = 0x0F,
    DMC_CTRL = 0x10,
    DMC_LOAD = 0x11,
    DMC_ADDR = 0x12,
    DMC_LEN = 0x13,
    _1 = 0x14,
    STATUS = 0x15,
    _2 = 0x16,
    FRAME_CNT = 0x17,
  };
  void write(CName r, uint8_t val, mapper::NESMapper &);
  uint8_t read(CName r, mapper::NESMapper &);

  uint8_t seqMode() const { return seq_mode_; }
  bool inhibitIrq() const { return inhibit_irq_; }

  bool reset() {
    static int count = 0;
    // delay frame control effects for a couple of cycles (maybe more?)
    if (!reset_) {
      count = 0;
    } else if (count == 2) {
      reset_ = false;
      seq_mode_ = (frame_control_reg_ & util::BIT7) >> 7;
      inhibit_irq_ = (frame_control_reg_ & util::BIT6);
      return true;
    } else {
      ++count;
    }
    return false;
    // auto tmp = reset_;
    // reset_ = false;
    // return tmp;
  }

  bool clearFrameInterrupt() {
    auto tmp = clear_frame_interrupt_;
    clear_frame_interrupt_ = false;
    return tmp;
  }

  bool p1Enable() const { return status_reg_ & util::BIT0; }
  bool p2Enable() const { return status_reg_ & util::BIT1; }
  bool trEnable() const { return status_reg_ & util::BIT2; }
  bool nsEnable() const { return status_reg_ & util::BIT3; }
  bool dmcEnable() const { return status_reg_ & util::BIT4; }

  bool dmcEnableChange() {
    auto tmp = dmc_en_changed;
    dmc_en_changed = false;
    return tmp;
  }

  bool p1EnvStart() {
    auto tmp = p1_env_start;
    p1_env_start = false;
    return tmp;
  }

  bool p2EnvStart() {
    auto tmp = p2_env_start;
    p2_env_start = false;
    return tmp;
  }

  bool nsEnvStart() {
    auto tmp = ns_env_start;
    ns_env_start = false;
    return tmp;
  }

  double p1Duty() const { return calc_duty_cycle(generator_regs[P1_CTRL]); }
  uint16_t p1Period() const {
    return ((static_cast<uint16_t>(generator_regs[P1_THI] & 0b111) << 8) |
            generator_regs[P1_TLO]);
  }

  uint8_t p1LcLoad() {
    if (p1_load_pending) {
      p1_load_pending = false;
      return (generator_regs[P1_THI] >> 3 & 0b11111);
    } else {
      return 0xFF;
    }
  }
  bool p1Halt() const { return generator_regs[P1_CTRL] & util::BIT5; }
  bool p1LoopEnv() const { return p1Halt(); }
  bool p1ConstVol() const { return generator_regs[P1_CTRL] & util::BIT4; }
  uint8_t p1VolDivider() const { return generator_regs[P1_CTRL] & 0b1111; }

  bool p1SweepEnabled() const { return generator_regs[P1_SWP] & util::BIT7; }
  uint8_t p1SweepDivider() {
    if (p1_sweep_reload) {
      p1_sweep_reload = false;
      return (generator_regs[P1_SWP] >> 4) & 0b111;
    } else {
      return 0xFF;
    }
  }
  bool p1SweepNegate() const { return generator_regs[P1_SWP] & util::BIT3; }
  uint8_t p1SweepShift() const { return generator_regs[P1_SWP] & 0b111; }

  bool p2SweepEnabled() const { return generator_regs[P2_SWP] & util::BIT7; }
  uint8_t p2SweepDivider() {
    if (p2_sweep_reload) {
      p2_sweep_reload = false;
      return (generator_regs[P2_SWP] >> 4) & 0b111;
    } else {
      return 0xFF;
    }
  }
  bool p2SweepNegate() const { return generator_regs[P2_SWP] & util::BIT3; }
  uint8_t p2SweepShift() const { return generator_regs[P2_SWP] & 0b111; }

  double p2Duty() const { return calc_duty_cycle(generator_regs[P2_CTRL]); }

  uint16_t p2Period() const {
    return ((static_cast<uint16_t>(generator_regs[P2_THI] & 0b111) << 8) |
            generator_regs[P2_TLO]);
  }

  uint8_t p2LcLoad() {
    if (p2_load_pending) {
      p2_load_pending = false;
      return (generator_regs[P2_THI] >> 3 & 0b11111);
    } else {
      return 0xFF;
    }
  }
  bool p2Halt() const { return generator_regs[P2_CTRL] & util::BIT5; }
  bool p2LoopEnv() const { return p2Halt(); }
  bool p2ConstVol() const { return generator_regs[P2_CTRL] & util::BIT4; }
  uint8_t p2VolDivider() const { return generator_regs[P2_CTRL] & 0b1111; }

  uint16_t trPeriod() const {
    return ((static_cast<uint16_t>(generator_regs[TR_THI] & 0b111) << 8) |
            generator_regs[TR_TLO]);
  }

  uint8_t trLcLoad() {
    if (tr_load_pending) {
      // if (!trHalt()) {
      tr_load_pending = false;
      // }
      return (generator_regs[TR_THI] >> 3 & 0b11111);
    } else {
      return 0xFF;
    }
  }

  uint8_t trLincLoad() {
    if (tr_lin_load_pending) {
      if (!trHalt()) {
        tr_lin_load_pending = false;
      }
      return (generator_regs[TR_CTRL] & 0b1111111);
    } else {
      return 0xFF;
    }
  }

  bool trHalt() const { return generator_regs[TR_CTRL] & util::BIT7; }

  bool nsMode() const { return generator_regs[NS_LNP] & util::BIT7; }

  double nsFreq() const {
    uint8_t period = (generator_regs[NS_LNP] & 0b1111);
    if (nsMode()) {
      return 4811.2 / (1 << period);
    } else {
      static double cpu_freq = 1.789773e6;
      return cpu_freq / (16 * (period + 1));
    }
  }

  bool nsHalt() const { return generator_regs[NS_CTRL] & util::BIT5; }
  bool nsLoopEnv() const { return nsHalt(); }
  bool nsConstVol() const { return generator_regs[NS_CTRL] & util::BIT4; }
  uint8_t nsVolDivider() const { return generator_regs[NS_CTRL] & 0b1111; }

  uint8_t nsLcLoad() {
    if (ns_load_pending) {
      ns_load_pending = false;
      return (generator_regs[NS_LCL] >> 3 & 0b11111);
    } else {
      return 0xFF;
    }
  }

  void setFcStatus(uint8_t s) { fc_status_ = s; }

  bool dmcInterruptEnable() const {
    return generator_regs[DMC_CTRL] & util::BIT7;
  }
  bool dmcLoop() const { return generator_regs[DMC_CTRL] & util::BIT6; }
  uint16_t dmcRate() const {
    return DMCRateTable[generator_regs[DMC_CTRL] & 0b1111];
  }
  uint8_t dmcDirectLoad() {
    if (dmc_direct_load) {
      dmc_direct_load = false;
      return generator_regs[DMC_LOAD] & 0x7F;
    } else {
      return 0xFF;
    }
  }
  uint16_t dmcSampleAddress() const {
    return 0xC000 + (static_cast<uint16_t>(generator_regs[DMC_ADDR]) << 6);
  }
  uint16_t dmcSampleLength() const {
    return (static_cast<uint16_t>(generator_regs[DMC_LEN]) << 4) + 1;
  }

private:
  double calc_duty_cycle(uint8_t val) const;

  std::array<uint8_t, 20> generator_regs = {};
  uint8_t status_reg_ = 0x00;
  uint8_t frame_control_reg_ = 0x00;

  // frame counter
  uint8_t seq_mode_ = 0;
  bool inhibit_irq_ = false;
  bool reset_ = false;
  uint8_t fc_status_ = 0x00;
  bool clear_frame_interrupt_ = false;

  bool p1_load_pending = true;
  bool p2_load_pending = true;
  bool tr_load_pending = true;
  bool tr_lin_load_pending = true;
  bool ns_load_pending = true;
  bool dmc_en_changed = false;

  bool dmc_direct_load = false;

  bool p1_sweep_reload = false;
  bool p2_sweep_reload = false;

  bool p1_env_start = false;
  bool p2_env_start = false;
  bool ns_env_start = false;

  static constexpr std::array<uint16_t, 0x10> DMCRateTable = {
      428, 380, 340, 320, 286, 254, 226, 214,
      190, 160, 142, 128, 106, 84,  72,  54,
  };
};

} // namespace aud
