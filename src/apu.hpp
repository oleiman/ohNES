#pragma once

#include "apu_registers.hpp"
#include "mappers/base_mapper.hpp"
// TODO(oren): I'd rather not include sdl code from here. maybe define the
// channel classes in the audio module instead?
#include "sdl/audio.hpp"

namespace aud {

struct LengthCounter {

  void tick() {
    if (halt_ || counter_ == 0) {
      return;
    } else {
      counter_--;
    }
  }
  bool load(uint8_t val) {
    if (enabled_ && val < 0x20) {
      counter_ = LengthTable[val & 0x1F];
    }
    return val < 0x20;
  }
  void linear_load(uint8_t val) { counter_ = val; }
  void halt(bool h) { halt_ = h; }
  void enable(bool e) {
    if (!e) {
      counter_ = 0;
    }
    enabled_ = e;
  }
  bool check() const { return counter_ > 0; };

  uint8_t curr() const { return counter_; }

private:
  uint8_t counter_ = 0;
  bool enabled_ = true;
  bool halt_ = false;
  static std::array<uint8_t, 0x20> LengthTable;
};

struct Envelope {
public:
  void tick(bool start);

  // double vol() const { return (constant_ ? v_ / 15.0 : divider_) & 0xF; }
  uint8_t vol() const { return (constant_ ? v_ : decay_level_) & 0xF; }

  void setVolDivider(uint8_t v) { v_ = v; }
  void setConst(bool c) { constant_ = c; }
  void setLoop(bool l) { loop_ = l; }
  void reset() { divider_ = v_; }

private:
  uint8_t divider_ = 0;
  uint8_t decay_level_ = 15;
  bool loop_ = false;
  bool constant_ = true;
  uint8_t v_ = 10;
};

struct Sweep {
public:
  void tick(uint16_t curr_period, int carry);
  int16_t change() { return change_amt_; }

  void setP(uint8_t val) {
    if (val < 0xFF) {
      p_ = val;
      should_reload_ = true;
    }
  }

  void enable(bool e) {
    if (!e) {
      change_amt_ = 0;
    }
    enabled_ = e;
  }

  void setNeg(bool n) { negative_ = n; }
  void setShift(uint8_t s) { shift_ = s; }
  bool shouldMute(int period) const {
    auto target = period + change_amt_;
    bool result = (target < 8 || target > 0x7FF);
    return result;
  }

private:
  int16_t change_amt_ = 0;
  uint8_t divider_ = 0;
  uint8_t p_ = 0;
  bool should_reload_ = false;
  uint8_t shift_ = 11;
  bool negative_ = false;
  bool enabled_ = false;
};

class Channel {
public:
  enum class Id {
    PULSE_1 = 0x00,
    PULSE_2 = 0x01,
    TRIANGLE = 0x02,
    NOISE = 0x03,
    DMC = 0x04,
  };
  Channel(Id id, sdl_internal::Generator &g, Registers &regs)
      : generator_(g), id_(id), regs_(regs) {}

  sdl_internal::Generator &generator() { return generator_; }

  void config() {
    bool e = false;
    uint8_t ld = 0xFF;
    bool h = false;
    double pitch = 0.0;
    bool loop_env = false;
    bool const_env = false;
    uint8_t v_env = 0;
    switch (id_) {
    case Id::PULSE_1:
      ld = regs_.p1LcLoad();
      mute_ = swp_.shouldMute(regs_.p1Period());
      pitch = calc_freq(regs_.p1Period() + swp_.change(), 16);
      h = regs_.p1Halt();
      e = regs_.p1Enable();
      loop_env = regs_.p1LoopEnv();
      const_env = regs_.p1ConstVol();
      v_env = regs_.p1VolDivider();
      swp_.enable(regs_.p1SweepEnabled());
      swp_.setP(regs_.p1SweepDivider());
      swp_.setNeg(regs_.p1SweepNegate());
      swp_.setShift(regs_.p1SweepShift());
      static_cast<sdl_internal::Pulse &>(generator_)
          .set_duty_cycle(regs_.p1Duty());
      break;
    case Id::PULSE_2:
      ld = regs_.p2LcLoad();
      mute_ = swp_.shouldMute(regs_.p2Period());
      pitch = calc_freq(regs_.p2Period() + swp_.change(), 16);
      h = regs_.p2Halt();
      e = regs_.p2Enable();
      loop_env = regs_.p2LoopEnv();
      const_env = regs_.p2ConstVol();
      v_env = regs_.p2VolDivider();
      swp_.enable(regs_.p2SweepEnabled());
      swp_.setP(regs_.p2SweepDivider());
      swp_.setNeg(regs_.p2SweepNegate());
      swp_.setShift(regs_.p2SweepShift());
      static_cast<sdl_internal::Pulse &>(generator_)
          .set_duty_cycle(regs_.p2Duty());
      break;
    case Id::TRIANGLE:
      ld = regs_.trLcLoad();
      pitch = calc_freq(regs_.trPeriod(), 32);
      h = regs_.trHalt();
      e = regs_.trEnable();
      const_env = true;
      v_env = 10;
      lin_lc_.enable(true);
      break;
    case Id::NOISE:
      ld = regs_.nsLcLoad();
      pitch = regs_.nsFreq();
      h = regs_.nsHalt();
      e = regs_.nsEnable();
      loop_env = regs_.nsLoopEnv();
      const_env = regs_.nsConstVol();
      v_env = regs_.nsVolDivider();
      break;
    default:
      std::cerr << "Channel not supported: " << static_cast<int>(id_)
                << std::endl;
      return;
    }
    lc_.enable(e);
    if (lc_.load(ld)) {
      // If we loaded a good value (i.e. lc load was set), reset the envelope
      env_.reset();
      // TODO(oren): reset pulse phase?
    }
    lc_.halt(h);
    env_.setVolDivider(v_env);
    env_.setConst(const_env);
    env_.setLoop(loop_env);
    if (pitch > 0) {
      generator_.set_pitch(pitch);
    }
  }

  void toggle();
  void syncVolume() { generator_.set_volume((1.0 * env_.vol()) / 15.0); }

  void tick_env() {
    switch (id_) {
    case Id::PULSE_1:
      env_.tick(regs_.p1EnvStart());
      break;
    case Id::PULSE_2:
      env_.tick(regs_.p2EnvStart());
      break;
    case Id::NOISE:
      env_.tick(regs_.nsEnvStart());
      break;
    default:
      break;
    }
    syncVolume();
  }
  void tick_len() { lc_.tick(); }
  void tick_sweep() {
    if (id_ == Id::PULSE_1) {
      swp_.tick(regs_.p1Period(), -1);
    } else if (id_ == Id::PULSE_2) {
      swp_.tick(regs_.p2Period(), 0);
    }
  }

  void tick_linear() {
    if (id_ == Id::TRIANGLE) {
      auto ld = regs_.trLincLoad();
      if (ld < 0xFF) {
        lin_lc_.linear_load(ld);
      } else {
        lin_lc_.tick();
      }
    }
  }

  bool isAudible() const {
    return checkLc() && checkLinc() && checkEnv() && !checkMute();
  }

  bool checkLinc() const { return (id_ != Id::TRIANGLE || lin_lc_.check()); }
  bool checkLc() const { return lc_.check(); }
  bool checkEnv() const { return (id_ == Id::TRIANGLE || env_.vol() > 0); }
  bool checkMute() const { return mute_ || force_mute_; }

  void forceMute(bool m) { force_mute_ = m; }

private:
  double calc_freq(uint16_t period, uint32_t m) const;

  sdl_internal::Generator &generator_;
  Id id_;
  Registers &regs_;
  LengthCounter lc_;
  LengthCounter lin_lc_;
  Envelope env_;
  Sweep swp_;
  bool mute_ = false;
  bool force_mute_ = false;
};

using Channels = std::unordered_map<Channel::Id, std::unique_ptr<Channel>>;

class Sequencer {
public:
  enum class Mode {
    M0 = 0,
    M1 = 1,
  };
  Sequencer() : mode_(Mode::M0) {}

  // returns true if we should generate an IRQ signal
  bool step(int cycle_count);
  void setMode(Mode m) { mode_ = m; }
  Mode mode() const { return mode_; }

  void reset() {
    if (mode_ == Mode::M1) {
      qf_ = true;
      hf_ = true;
    }
  }

  void clock(Channels &channels) {
    if (qf_) {
      qf_ = false;
      quarter_frame(channels);
    }
    if (hf_) {
      hf_ = false;
      half_frame(channels);
    }
  }

private:
  void quarter_frame(Channels &channels);
  void half_frame(Channels &channels);
  Mode mode_;
  bool qf_ = false;
  bool hf_ = false;
};

class FrameCounter {
public:
  FrameCounter() = default;

  void inc(Channels &channels, aud::Registers &regs);

  uint8_t status(const Channels &channels) const;
  // TODO(oren): when do we clear this?
  bool frameInterrupt() const { return frame_interrupt_.status; }

  int count() const { return counter_; }

private:
  Sequencer seq_;
  int counter_;
  // bool frame_interrupt_ = false;
  struct {
    bool status = false;
    int count;
    void set(int cnt) {
      status = true;
      count = cnt;
    }

    void clear() { status = false; }

    void dec() {
      if (count > 0) {
        --count;
        status = true;
      }
    }

  } frame_interrupt_;
};

class APU {

public:
  APU(mapper::NESMapper &mapper, Registers &registers)
      : mapper_(mapper), registers_(registers) {
    (void)mapper_;
    (void)registers_;
    channels_.emplace(
        Channel::Id::PULSE_1,
        std::make_unique<Channel>(
            Channel::Id::PULSE_1,
            sdl_internal::Audio::MakeGenerator<sdl_internal::Pulse>(),
            registers_));
    channels_.emplace(
        Channel::Id::PULSE_2,
        std::make_unique<Channel>(
            Channel::Id::PULSE_2,
            sdl_internal::Audio::MakeGenerator<sdl_internal::Pulse>(),
            registers_));
    channels_.emplace(
        Channel::Id::TRIANGLE,
        std::make_unique<Channel>(
            Channel::Id::TRIANGLE,
            sdl_internal::Audio::MakeGenerator<sdl_internal::Triangle>(),
            registers_));
    channels_.emplace(
        Channel::Id::NOISE,
        std::make_unique<Channel>(
            Channel::Id::NOISE,
            sdl_internal::Audio::MakeGenerator<sdl_internal::Noise>(),
            registers_));
  }

  void step(bool &irq);

  void mute(int cid, bool m) {
    Channel::Id id{cid};
    auto it = channels_.find(id);
    if (it != channels_.end()) {
      it->second->forceMute(m);
    }
  }

private:
  mapper::NESMapper &mapper_;
  Registers &registers_;
  Channels channels_;
  FrameCounter frame_counter_;
};

} // namespace aud
