#pragma once

#include "dbg/nes_debugger.hpp"
#include "util.hpp"

namespace aud {

enum class ChannelId {
  PULSE_1 = 0x00,
  PULSE_2 = 0x01,
  TRIANGLE = 0x02,
  NOISE = 0x03,
  DMC = 0x04,
  NCID,
};

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
    N_REGS = 0x18,
  };
  void write(CName r, uint8_t val, mapper::NESMapper &);
  uint8_t read(CName r, mapper::NESMapper &);
  void reload(CName r, mapper::NESMapper &);

  uint8_t seqMode() const { return seq_mode_; }
  bool inhibitIrq() const { return inhibit_irq_; }

  bool frameCounterReset() {
    static int count = 0;
    // delay frame control effects for a couple of cycles (maybe more?)
    if (!fc_reset_) {
      count = 0;
      return false;
    }

    if (count == 2) {
      fc_reset_ = false;
      seq_mode_ = (frame_control_reg_ & util::BIT7) >> 7;
      inhibit_irq_ = (frame_control_reg_ & util::BIT6);
      return true;
    } else {
      ++count;
      return false;
    }
  }

  bool clearFrameInterrupt() {
    return get_and_clear_flag(clear_frame_interrupt_);
  }

  bool isEnabled(ChannelId id) {
    return status_reg_ & (0b1 << static_cast<uint8_t>(id));
  }

  bool dmcEnableChange() { return get_and_clear_flag(dmc_en_changed); }

  bool envStart(ChannelId id) {
    return get_and_clear_channel_flag(EnvStartFlags, id);
  }
  uint8_t lcLoad(ChannelId id) {
    if (get_and_clear_channel_flag(LcLoadFlags, id)) {
      return get_reg(id, 3) >> 3 & 0b11111;
    } else {
      return 0xFF;
    }
  }
  bool lcHalt(ChannelId id) {
    return get_reg(id, 0) & (is_tri(id) ? util::BIT7 : util::BIT5);
  }
  bool loopEnv(ChannelId id) { return !is_tri(id) && lcHalt(id); }
  bool constVol(ChannelId id) {
    return is_tri(id) || (get_reg(id, 0) & util::BIT4);
  }

  uint8_t volDivider(ChannelId id) {
    if (is_tri(id)) {
      return 10;
    } else {
      return get_reg(id, 0) & 0b1111;
    }
  }

  bool sweepEnabled(ChannelId id) {
    return is_pulse(id) && (get_reg(id, 1) & util::BIT7);
  }
  uint8_t sweepDivider(ChannelId id) {
    if (get_and_clear_channel_flag(SweepReloadFlags, id)) {
      return (get_reg(id, 1) >> 4) & 0b111;
    } else {
      return 0xFF;
    }
  }
  bool sweepNegate(ChannelId id) { return get_reg(id, 1) & util::BIT3; }
  uint8_t sweepShift(ChannelId id) { return get_reg(id, 1) & 0b111; }

  double dutyCycle(ChannelId id) {
    assert(is_pulse(id));
    return calc_duty_cycle(get_reg(id, 0));
  }

  uint16_t generatorPeriod(ChannelId id) {
    assert(!is_noise(id));
    return ((static_cast<uint16_t>(get_reg(id, 3) & 0b111) << 8) |
            get_reg(id, 2));
  }

  uint8_t trLincLoad() {
    if (tr_lin_load_pending) {
      if (!lcHalt(ChannelId::TRIANGLE)) {
        tr_lin_load_pending = false;
      }
      return (generator_regs[TR_CTRL] & 0b1111111);
    } else {
      return 0xFF;
    }
  }

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

  using ChannelFlags = std::array<bool, static_cast<int>(ChannelId::NCID)>;

  void set_channel_flag(ChannelFlags &arr, ChannelId id) {
    arr[static_cast<int>(id)] = true;
  }

  bool get_and_clear_channel_flag(ChannelFlags &arr, ChannelId id) {
    return get_and_clear_flag(arr[static_cast<int>(id)]);
  }

  bool get_and_clear_flag(bool &flag) {
    bool tmp = flag;
    flag = false;
    return tmp;
  }

  uint8_t get_reg(ChannelId id, uint8_t off) {
    return generator_regs[GRegBase[static_cast<int>(id)] + off];
  }

  bool is_pulse(ChannelId id) {
    return id == ChannelId::PULSE_1 || id == ChannelId::PULSE_2;
  }
  bool is_tri(ChannelId id) { return id == ChannelId::TRIANGLE; }
  bool is_noise(ChannelId id) { return id == ChannelId::NOISE; }
  bool is_dmc(ChannelId id) { return id == ChannelId::DMC; }

  std::array<uint8_t, 20> generator_regs = {};
  uint8_t status_reg_ = 0x00;

  // frame counter
  uint8_t frame_control_reg_ = 0x00;
  bool fc_reset_ = false;
  uint8_t fc_status_ = 0x00;
  bool clear_frame_interrupt_ = false;
  uint8_t seq_mode_ = 0;
  bool inhibit_irq_ = false;

  bool tr_lin_load_pending = true;

  bool dmc_en_changed = false;
  bool dmc_direct_load = false;

  std::array<uint8_t, CName::N_REGS> last_write_ = {0};

  static constexpr std::array<uint16_t, 0x10> DMCRateTable = {
      428, 380, 340, 320, 286, 254, 226, 214,
      190, 160, 142, 128, 106, 84,  72,  54,
  };

  static constexpr std::array<uint8_t, static_cast<size_t>(ChannelId::NCID)>
      GRegBase = {0x00, 0x04, 0x08, 0x0C, 0x10};
  static ChannelFlags EnvStartFlags;
  static ChannelFlags LcLoadFlags;
  static ChannelFlags SweepReloadFlags;
};

} // namespace aud
