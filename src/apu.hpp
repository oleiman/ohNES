#pragma once

#include "apu_registers.hpp"
#include "gen_audio.hpp"
#include "mappers/base_mapper.hpp"

#include <unordered_map>
#include <unordered_set>

namespace aud {

struct LengthCounter {

  void tick();
  bool load(uint8_t val);
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

  bool config(Registers &regs, ChannelId id);

private:
  uint8_t counter_ = 0;
  bool enabled_ = true;
  bool halt_ = false;
  static const std::array<uint8_t, 0x20> LengthTable;
};

struct Envelope {
  void tick(bool start);
  uint8_t vol() const { return (constant_ ? v_ : decay_level_) & 0xF; }
  void setVolDivider(uint8_t v) { v_ = v; }
  void setConst(bool c) { constant_ = c; }
  void setLoop(bool l) { loop_ = l; }
  void reset() { divider_ = v_; }
  void config(Registers &regs, ChannelId id);

private:
  uint8_t divider_ = 0;
  uint8_t decay_level_ = 15;
  bool loop_ = false;
  bool constant_ = true;
  uint8_t v_ = 10;
};

struct Sweep {

public:
  Sweep() = delete;
  explicit Sweep(int carry) : carry_(carry) {}
  void tick(uint16_t curr_period);
  int16_t change() const { return change_amt_; }

  void setP(uint8_t val) {
    if (val < 0xFF) {
      p_ = val;
      should_reload_ = true;
    }
  }

  void enable(bool e) {
    // TODO(oren): need something more sophisticated to reset the sweep change
    // amount
    if (!e) {
      change_amt_ = 0;
    }
    enabled_ = e;
  }

  void reset() { change_amt_ = 0; }

  void setNeg(bool n) { negative_ = n; }
  void setShift(uint8_t s) { shift_ = s; }
  bool shouldMute(int period) const {
    auto target = period + change_amt_;
    return (target < 8 || target > 0x7FF);
  }

  void config(Registers &regs, ChannelId id) {
    enable(regs.sweepEnabled(id));
    setP(regs.sweepDivider(id));
    setNeg(regs.sweepNegate(id));
    setShift(regs.sweepShift(id));
  }

private:
  int16_t change_amt_ = 0;
  uint8_t divider_ = 0;
  uint8_t p_ = 0;
  bool should_reload_ = false;
  uint8_t shift_ = 11;
  bool negative_ = false;
  bool enabled_ = false;
  int carry_ = 0;
};

class Channel {
public:
  Channel() = delete;
  Channel(ChannelId id, Generator &g, Registers &regs)
      : generator_(g), id_(id), regs_(regs),
        swp_(id == ChannelId::PULSE_1 ? -1 : 0) {}

  bool isPulse() const {
    return (id_ == ChannelId::PULSE_1 || id_ == ChannelId::PULSE_2);
  }
  bool isTriangle() const { return id_ == ChannelId::TRIANGLE; }
  bool isNoise() const { return id_ == ChannelId::NOISE; }
  void config();
  void toggle();
  void syncVolume();

  void tick_env() {
    env_.tick(regs_.envStart(id_));
    syncVolume();
  }
  void tick_len() { lc_.tick(); }
  void tick_sweep() {
    if (isPulse()) {
      swp_.tick(regs_.generatorPeriod(id_));
    }
  }

  void tick_linear() {
    if (isTriangle()) {
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

  uint8_t status() const {
    return ((checkLc() ? 0b1 : 0b0) << static_cast<uint8_t>(id_));
  }

  void forceMute(bool m) { force_mute_ = m; }

private:
  double getPitch() const {
    if (isNoise()) {
      return regs_.nsFreq();
    } else if (isPulse()) {
      return calc_freq(regs_.generatorPeriod(id_) + swp_.change());
    } else {
      return calc_freq(regs_.generatorPeriod(id_));
    }
  }

  bool checkLinc() const { return (!isTriangle() || lin_lc_.check()); }
  bool checkLc() const { return lc_.check(); }
  bool checkEnv() const { return (isTriangle() || env_.vol() > 0); }
  bool checkMute() const { return mute_ || force_mute_; }

  double calc_freq(uint16_t period) const;

  Generator &generator_;
  ChannelId id_;
  Registers &regs_;
  LengthCounter lc_;
  LengthCounter lin_lc_;
  Envelope env_;
  Sweep swp_;
  bool mute_ = false;
  bool force_mute_ = false;
};

using Channels = std::unordered_map<ChannelId, std::unique_ptr<Channel>>;

struct SampleBuffer {
  SampleBuffer(mapper::NESMapper &mapper, Registers &regs)
      : mapper_(mapper), regs_(regs) {}

  void enable();
  void disable() { bytes_remaining_ = 0; }
  bool empty() const { return empty_; }
  uint16_t bytesRemaining() const { return bytes_remaining_; }

  uint8_t get();

  bool isFirst() {
    auto tmp = is_first_byte_;
    is_first_byte_ = false;
    return tmp;
  }

private:
  void fill();

  mapper::NESMapper &mapper_;
  Registers &regs_;
  uint8_t buf_ = 0;
  uint16_t sample_base_addr_;
  uint16_t sample_len_;
  uint16_t curr_addr_;
  uint16_t bytes_remaining_ = 0;
  bool empty_ = true;
  bool sample_load_ = false;
  bool is_first_byte_ = false;
};

struct SampleOutput {
  SampleOutput() {}

  uint8_t clock(SampleBuffer &sbuf);

  void setLevel(uint8_t lvl) {
    if (lvl < 0xFF) {
      level_setting_ = lvl;
      output_level_ = lvl;
    }
  }

  void enable() { enable_ = true; }
  void disable() {
    level_setting_ = 0;
    output_level_ = 0x40;
    enable_ = false;
  }

  bool isEnabled() const { return enable_; }

private:
  void start_cycle(SampleBuffer &sbuf);
  uint8_t shift_register_ = 0;
  uint8_t output_level_ = 0x40;
  uint8_t level_setting_ = 0;
  uint8_t bits_remaining_ = 8;
  bool silence_ = true;
  bool enable_ = false;
};

struct SampleTimer {
  void setPeriod(uint16_t per) { period_ = (per >> 1); }
  uint16_t period() { return (period_ << 1); }
  bool tick();

private:
  uint16_t counter_ = 0;
  uint16_t period_;
};

class DMCUnit {
public:
  DMCUnit(DMC &gen, Registers &regs, mapper::NESMapper &mapper)
      : gen_(gen), regs_(regs), sbuf_(mapper, regs) {}

  void config();
  bool step();
  uint16_t bytesRemaining() const { return sbuf_.bytesRemaining(); }

private:
  double calc_freq(uint16_t period) const;
  DMC &gen_;
  Registers &regs_;
  SampleBuffer sbuf_;
  SampleOutput output_;
  SampleTimer timer_;
  ChannelId id_ = ChannelId::DMC;
};

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

  void clock(Channels &channels);

private:
  void quarter_frame(Channels &channels);
  void half_frame(Channels &channels);
  Mode mode_;
  bool qf_ = false;
  bool hf_ = false;
  static const std::unordered_map<Mode, std::unordered_set<int>> QfLookup;
  static const std::unordered_map<Mode, std::unordered_set<int>> HfLookup;
};

class FrameCounter {
public:
  FrameCounter() = default;

  void inc(Channels &channels, DMCUnit &dmc, aud::Registers &regs);

  uint8_t status(const Channels &channels, const DMCUnit &dmc) const;
  // TODO(oren): when do we clear this?
  bool frameInterrupt() const { return frame_interrupt_.status; }
  bool dmcInterrupt() const { return dmc_interrupt_; }

  int count() const { return counter_; }

private:
  Sequencer seq_;
  int counter_;
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
  bool dmc_interrupt_ = false;
};

class APU {

public:
  APU(mapper::NESMapper &mapper, Registers &registers);
  void step(bool &irq);
  void mute(int cid, bool m) {
    ChannelId id{cid};
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
  std::unique_ptr<DMCUnit> dmc_unit_;
};

} // namespace aud
