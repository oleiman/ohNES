#include "apu.hpp"
#include "util.hpp"

namespace aud {

void APU::step(bool &irq) {

  frame_counter_.inc(channels_, registers_);

  // NOTE(oren): actually setting the IRQ here produces weird behavior
  // not really sure how this should be treated
  // irq = frame_counter_.frameInterrupt();

  registers_.setFcStatus(frame_counter_.status(channels_));
}

void FrameCounter::inc(Channels &channels, aud::Registers &regs) {

  // configure sound generators

  if (regs.clearFrameInterrupt()) {
    frame_interrupt_.clear();
  }

  frame_interrupt_.dec();

  for (auto &[id, chan] : channels) {
    chan->config();
  }

  if (regs.reset()) {

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
  } else {
    seq_.clock(channels);
  }

  alter = !alter;

  for (auto &[id, chan] : channels) {
    chan->toggle();
  }
}

uint8_t FrameCounter::status(const Channels &channels) const {
  uint8_t result = 0;
  if (frame_interrupt_.status) {
    result |= util::BIT6;
  }
  // if (dmc_interrupt_) {
  //   result |= util::BIT7;
  // }
  for (const auto &[id, chan] : channels) {
    if (chan->checkLc() && id != Channel::Id::DMC) {
      result |= (0b1 << static_cast<uint8_t>(id));
    }
  }
  return result;
}

bool Sequencer::step(int cycle_count) {
  bool irq_required = false;

  if (mode_ == Mode::M0) {
    switch (cycle_count) {
    // case 0:
    //   irq_required = true;
    //   break;
    case 3728:
      qf_ = true;
      break;
    case 7456:
      qf_ = true;
      hf_ = true;
      break;
    case 11185:
      qf_ = true;
      break;
    case 14914:
      irq_required = true;
      qf_ = true;
      hf_ = true;
      break;
    default:
      break;
    }
  } else {
    switch (cycle_count) {
    case 3728:
      qf_ = true;
      break;
    case 7456:
      qf_ = true;
      hf_ = true;
      break;
    case 11185:
      qf_ = true;
      break;
    case 14914:
      break;
    case 18640:
      qf_ = true;
      hf_ = true;
      break;
    default:
      break;
    }
  }

  return irq_required;
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

void Sweep::tick(uint16_t curr_period, int carry) {
  if (divider_ == 0 && enabled_ && !shouldMute(curr_period)) {
    int16_t amt = static_cast<int16_t>((curr_period + change_amt_) >> shift_);
    assert(amt >= 0);
    if (negative_) {
      amt = -amt + carry;
    }

    // std::cout << amt << std::endl;

    change_amt_ += amt;
  }

  if (should_reload_ || divider_ == 0) {
    divider_ = p_;
    should_reload_ = false;
  } else {
    --divider_;
  }
}

void Channel::toggle() {
  if (isAudible()) {
    generator_.on();
  } else {
    generator_.off();
  }
}

// TODO(oren): pass the channel type rather than m
// this should be a general goal for the registers module
double Channel::calc_freq(uint16_t period, uint32_t m) const {
  static double cpu_freq = 1.789773e6;
  return cpu_freq / (m * (period + 1));
}

std::array<uint8_t, 0x20> LengthCounter::LengthTable = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14, // 00-0F
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30, // 10-1F
};

} // namespace aud
