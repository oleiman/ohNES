#include "apu.hpp"
#include "util.hpp"

namespace aud {

APU::APU(mapper::NESMapper &mapper, Registers &registers)
    : mapper_(mapper), registers_(registers) {
  (void)mapper_;
  (void)registers_;
  channels_.emplace(ChannelId::PULSE_1,
                    std::make_unique<Channel>(ChannelId::PULSE_1,
                                              Generator::Make<Pulse>(),
                                              registers_));
  channels_.emplace(ChannelId::PULSE_2,
                    std::make_unique<Channel>(ChannelId::PULSE_2,
                                              Generator::Make<Pulse>(),
                                              registers_));
  channels_.emplace(ChannelId::TRIANGLE,
                    std::make_unique<Channel>(ChannelId::TRIANGLE,
                                              Generator::Make<Triangle>(),
                                              registers_));
  channels_.emplace(ChannelId::NOISE,
                    std::make_unique<Channel>(ChannelId::NOISE,
                                              Generator::Make<Noise>(),
                                              registers_));
  dmc_unit_ =
      std::make_unique<DMCUnit>(Generator::Make<DMC>(), registers_, mapper_);
}

void APU::step(bool &irq) {

  frame_counter_.inc(channels_, *dmc_unit_, registers_);

  // NOTE(oren): actually setting the IRQ here produces weird behavior
  // not really sure how this should be treated
  // irq = frame_counter_.frameInterrupt();

  registers_.setFcStatus(frame_counter_.status(channels_, *dmc_unit_));

  // if (frame_counter_.dmcInterrupt()) {
  //   irq = true;
  // }
}

void FrameCounter::inc(Channels &channels, DMCUnit &dmc_unit,
                       aud::Registers &regs) {

  if (regs.clearFrameInterrupt()) {
    frame_interrupt_.clear();
  }

  frame_interrupt_.dec();

  for (auto &[id, chan] : channels) {
    chan->config();
  }

  if (regs.dmcEnableChange()) {
    dmc_unit.config();
    dmc_interrupt_ = false;
  }

  if (!regs.dmcInterruptEnable()) {
    dmc_interrupt_ = false;
  }

  if (regs.frameCounterReset()) {

    seq_.setMode(Sequencer::Mode(regs.seqMode()));
    seq_.reset();

    if (regs.inhibitIrq()) {
      frame_interrupt_.clear();
    }
    counter_ = 0;
  }

  static bool alter = true;
  if (alter) {
    if (seq_.step(counter_) && !regs.inhibitIrq()) {
      // set the frame interrupt flag on this and the next two CPU cycles
      frame_interrupt_.set(2);
    }
    ++counter_;
    if ((seq_.mode() == Sequencer::Mode::M0 && counter_ == 14915) ||
        (seq_.mode() == Sequencer::Mode::M1 && counter_ == 18641)) {
      counter_ = 0;
    }

    if (dmc_unit.step() && regs.dmcInterruptEnable()) {
      dmc_interrupt_ = true;
    }
  } else {
    seq_.clock(channels);
  }

  alter = !alter;

  for (auto &[id, chan] : channels) {
    chan->toggle();
  }
}

uint8_t FrameCounter::status(const Channels &channels,
                             const DMCUnit &dmc_unit) const {
  uint8_t result = 0;
  if (dmc_interrupt_) {
    result |= util::BIT7;
  }

  if (frame_interrupt_.status) {
    result |= util::BIT6;
  }

  for (const auto &[id, chan] : channels) {
    result |= chan->status();
  }

  if (dmc_unit.bytesRemaining()) {
    result |= 0b1 << 4;
  }

  return result;
}

void LengthCounter::tick() {
  if (halt_ || counter_ == 0) {
    return;
  } else {
    counter_--;
  }
}
bool LengthCounter::load(uint8_t val) {
  if (enabled_ && val < 0x20) {
    counter_ = LengthTable[val & 0x1F];
  }
  return val < 0x20;
}

bool LengthCounter::config(Registers &regs, ChannelId id) {
  bool reset_env = false;
  enable(regs.isEnabled(id));
  if (load(regs.lcLoad(id))) {
    // If we loaded a good value (i.e. lc load was set), reset the envelope
    reset_env = true;
  }
  halt(regs.lcHalt(id));
  return reset_env;
}

void Envelope::config(Registers &regs, ChannelId id) {
  setVolDivider(regs.volDivider(id));
  setConst(regs.constVol(id));
  setLoop(regs.loopEnv(id));
}

void SampleBuffer::enable() {
  if (bytes_remaining_ == 0) {
    sample_base_addr_ = curr_addr_ = regs_.dmcSampleAddress();
    sample_len_ = bytes_remaining_ = regs_.dmcSampleLength();
    sample_load_ = true;
    fill();
  }
}

uint8_t SampleBuffer::get() {
  assert(!empty_);
  auto val = buf_;
  if (sample_load_) {
    is_first_byte_ = true;
    sample_load_ = false;
  }
  fill();
  return val;
}

void SampleBuffer::fill() {
  if (bytes_remaining_ > 0) {
    empty_ = false;
    buf_ = mapper_.read(curr_addr_);
    if (curr_addr_ == 0xFFFF) {
      curr_addr_ = 0x8000;
    } else {
      ++curr_addr_;
    }
    --bytes_remaining_;
    if (bytes_remaining_ == 0 && regs_.dmcLoop()) {
      curr_addr_ = regs_.dmcSampleAddress();
      bytes_remaining_ = regs_.dmcSampleLength();
      sample_load_ = true;
    }
  } else {
    empty_ = true;
  }
}

uint8_t SampleOutput::clock(SampleBuffer &sbuf) {
  if (!silence_) {
    uint8_t l = shift_register_ & 0b1;
    if (l && output_level_ <= 125) {
      output_level_ += 2;
    } else if (!l && output_level_ >= 2) {
      output_level_ -= 2;
    }
  }
  shift_register_ >>= 1;
  --bits_remaining_;
  if (bits_remaining_ == 0) {
    start_cycle(sbuf);
  }
  return (silence_ ? 0xFF : output_level_);
}

void SampleOutput::start_cycle(SampleBuffer &sbuf) {
  silence_ = sbuf.empty();
  if (!silence_) {
    shift_register_ = sbuf.get();
    if (sbuf.isFirst()) {
      output_level_ = 0x40;
    }
  }
  bits_remaining_ = 8;
}

bool SampleTimer::tick() {
  ++counter_;
  if (counter_ >= period_) {
    counter_ = 0;
    return true;
  } else {
    return false;
  }
}

void DMCUnit::config() {
  timer_.setPeriod(regs_.dmcRate());
  output_.setLevel(regs_.dmcDirectLoad());

  if (regs_.isEnabled(id_)) {
    sbuf_.enable();
    output_.enable();
    gen_.on();
  } else {
    sbuf_.disable();
    output_.disable();
    gen_.off();
  }
}

bool DMCUnit::step() {
  auto br_prev = bytesRemaining();
  if (timer_.tick()) {
    auto out_lvl = output_.clock(sbuf_);
    assert(out_lvl <= 127 || out_lvl == 0xFF);
    gen_.change_output_level(out_lvl, timer_.period());
  }
  return (br_prev > 0 && bytesRemaining() == 0);
}

bool Sequencer::step(int cycle_count) {
  bool irq_required = (mode_ == Mode::M0 && cycle_count == 14914);

  const auto qlt = QfLookup.find(mode_);
  assert(qlt != QfLookup.end());
  qf_ = qf_ || qlt->second.find(cycle_count) != qlt->second.end();

  const auto hlt = HfLookup.find(mode_);
  assert(hlt != HfLookup.end());
  hf_ = hf_ || hlt->second.find(cycle_count) != hlt->second.end();

  return irq_required;
}

void Sequencer::clock(Channels &channels) {
  if (qf_) {
    qf_ = false;
    quarter_frame(channels);
  }
  if (hf_) {
    hf_ = false;
    half_frame(channels);
  }
}

void Sequencer::quarter_frame(Channels &channels) {
  for (auto &[id, c] : channels) {
    c->tick_linear();
    c->tick_env();
  }
}
void Sequencer::half_frame(Channels &channels) {
  for (auto &[id, c] : channels) {
    c->tick_len();
    c->tick_sweep();
  }
}

const std::unordered_map<Sequencer::Mode, std::unordered_set<int>>
    Sequencer::QfLookup = {
        {Mode::M0, {3728, 7456, 11185, 14914}},
        {Mode::M1, {3728, 7456, 11185, 18640}},
};

const std::unordered_map<Sequencer::Mode, std::unordered_set<int>>
    Sequencer::HfLookup = {
        {Mode::M0, {7456, 14914}},
        {Mode::M1, {7456, 18640}},
};

void Envelope::tick(bool start) {
  if (start) {
    decay_level_ = 15;
    divider_ = v_;
    return;
  } else if (divider_ == 0) {
    // reload divider
    divider_ = v_;
    // clock the decay level
    if (decay_level_ > 0) {
      --decay_level_;
    } else if (loop_) {
      decay_level_ = 15;
    }
  } else {
    // clock the divider
    --divider_;
  }
}

void Sweep::tick(uint16_t curr_period) {
  if (divider_ == 0 && enabled_ && !shouldMute(curr_period)) {
    int16_t amt = static_cast<int16_t>((curr_period + change_amt_) >> shift_);
    assert(amt >= 0);
    if (negative_) {
      amt = -amt + carry_;
    }

    change_amt_ += amt;
  }

  if (should_reload_) {
    // TODO(oren): this isn't quite right. Fixes an issue where successive swept
    // sounds will rise in pitch indefinitely, but introduces incorrect pitch
    // behavior in, e.g. the SMB death tune. Also fixes an issue with the big
    // mario to little mario transition sound effect.
    reset();
    divider_ = 0;
    should_reload_ = false;
  }

  if (divider_ == 0) {
    divider_ = p_;
  } else {
    --divider_;
  }
}

void Channel::config() {
  if (isTriangle()) {
    lin_lc_.enable(true);
  } else if (isPulse()) {
    mute_ = swp_.shouldMute(regs_.generatorPeriod(id_));
    swp_.config(regs_, id_);
    static_cast<Pulse &>(generator_).set_duty_cycle(regs_.dutyCycle(id_));
  }

  if (lc_.config(regs_, id_)) {
    env_.reset();
    // TODO(oren): reset pulse phase?
  }

  env_.config(regs_, id_);
  generator_.set_pitch(getPitch());
}

void Channel::toggle() {
  if (isAudible()) {
    generator_.on();
  } else {
    generator_.off();
  }
}
void Channel::syncVolume() { generator_.set_volume((1.0 * env_.vol()) / 15.0); }

// TODO(oren): pass the channel type rather than m
// this should be a general goal for the registers module
double Channel::calc_freq(uint16_t period) const {
  uint32_t m = isPulse() ? 16 : 32;
  static double cpu_freq = 1.789773e6;
  return cpu_freq / (m * (period + 1));
}

double DMCUnit::calc_freq(uint16_t period) const {
  static double cpu_freq = 1.789773e6;
  return cpu_freq / period;
}

const std::array<uint8_t, 0x20> LengthCounter::LengthTable = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14, // 00-0F
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30, // 10-1F
};

// const std::array<double, 0x10> DMCUnit::PeriodTable;

} // namespace aud
